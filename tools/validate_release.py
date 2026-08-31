"""Fail fast when package and native product versions drift before a release."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SEMANTIC_VERSION = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+")


def require_match(path: str, pattern: str) -> str:
    content = (ROOT / path).read_text(encoding="utf-8")
    match = re.search(pattern, content, flags=re.MULTILINE)
    if match is None:
        raise ValueError(f"could not read product version from {path}")
    return match.group(1)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "version",
        nargs="?",
        help="release version without a v prefix; defaults to the CMake version",
    )
    arguments = parser.parse_args()

    cmake_version = require_match(
        "CMakeLists.txt", r"project\(Ratatoskr VERSION ([0-9.]+)"
    )
    release_version = arguments.version or cmake_version
    if SEMANTIC_VERSION.fullmatch(release_version) is None:
        print(
            f"invalid release version {release_version!r}; expected MAJOR.MINOR.PATCH",
            file=sys.stderr,
        )
        return 2

    header = (ROOT / "include/ratatoskr/version.h").read_text(encoding="utf-8")
    native_parts = []
    for component in ("MAJOR", "MINOR", "PATCH"):
        match = re.search(rf"#define RATOS_VERSION_{component} ([0-9]+)u", header)
        if match is None:
            print(f"could not read RATOS_VERSION_{component}", file=sys.stderr)
            return 1
        native_parts.append(match.group(1))

    versions = {
        "CMake": cmake_version,
        "C header": ".".join(native_parts),
        "Java": require_match("bindings/java/pom.xml", r"<revision>([^<]+)</revision>"),
        "Kotlin": require_match("bindings/kotlin/pom.xml", r"<revision>([^<]+)</revision>"),
        "Python": require_match(
            "bindings/python/src/ratatoskr/_version.py",
            r'^__version__ = "([^"]+)"$',
        ),
        "Rust": require_match("bindings/rust/Cargo.toml", r'^version = "([^"]+)"$'),
        "npm": require_match("bindings/node/package.json", r'^\s*"version": "([^"]+)"'),
        "Packagist": require_match("bindings/php/composer.json", r'^\s*"version": "([^"]+)"'),
        "RubyGems": require_match("bindings/ruby/ratatoskr.gemspec", r'spec\.version = "([^"]+)"'),
        "pub.dev": require_match("bindings/dart/pubspec.yaml", r'^version: ([^\s]+)$'),
        "LuaRocks": require_match("bindings/lua/ratatoskr-0.1.0-1.rockspec", r'^version = "([0-9.]+)-[0-9]+"$'),
        "Conan": require_match("conanfile.py", r'^\s*version = "([^"]+)"$'),
        "Arch": require_match("packaging/arch/PKGBUILD", r"^pkgver=([^\s]+)$"),
        "man page": require_match("docs/man/ratos.1", r'"Ratatoskr ([^"]+)"'),
    }
    mismatches = {
        component: version
        for component, version in versions.items()
        if version != release_version
    }
    if mismatches:
        print(
            f"release version is {release_version}, but sources disagree:",
            file=sys.stderr,
        )
        for component, version in mismatches.items():
            print(f"  {component}: {version}", file=sys.stderr)
        return 1

    print(
        f"release version {release_version} is consistent across {len(versions)} sources"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
