"""Build one native-backed wheel per release platform plus an sdist."""

from __future__ import annotations

import argparse
import runpy
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

PLATFORMS = (
    ("linux-x64", "linux-x86_64", "libratatoskr.so", "linux_x86_64"),
    ("linux-arm64", "linux-aarch64", "libratatoskr.so", "linux_aarch64"),
    ("osx-x64", "macos-x86_64", "libratatoskr.dylib", "macosx_11_0_x86_64"),
    ("osx-arm64", "macos-aarch64", "libratatoskr.dylib", "macosx_11_0_arm64"),
    ("win-x64", "windows-x86_64", "ratatoskr.dll", "win_amd64"),
    ("win-arm64", "windows-aarch64", "ratatoskr.dll", "win_arm64"),
)


def _clear_build_state(package_root: Path, native_root: Path) -> None:
    for _, directory, _, _ in PLATFORMS:
        shutil.rmtree(native_root / directory, ignore_errors=True)
    shutil.rmtree(package_root / "build", ignore_errors=True)
    for egg_info in (package_root / "src").glob("*.egg-info"):
        shutil.rmtree(egg_info, ignore_errors=True)


def _run_build(package_root: Path, output: Path, arguments: list[str]) -> None:
    subprocess.run(
        [
            sys.executable,
            "-m",
            "build",
            "--quiet",
            "--no-isolation",
            "--outdir",
            str(output),
            *arguments,
            str(package_root),
        ],
        check=True,
    )


def _validate_wheel(path: Path, expected_library: str) -> None:
    with zipfile.ZipFile(path) as archive:
        libraries = [
            name
            for name in archive.namelist()
            if name.endswith((".so", ".dylib", ".dll"))
        ]
    if len(libraries) != 1 or not libraries[0].endswith(expected_library):
        raise RuntimeError(
            f"{path.name} must contain exactly {expected_library}; got {libraries}"
        )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--native-stage", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--version", required=True)
    arguments = parser.parse_args()

    package_root = Path(__file__).resolve().parents[1]
    native_root = package_root / "src" / "ratatoskr" / "_native"
    output = arguments.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if any(output.iterdir()):
        raise RuntimeError(f"release output directory must be empty: {output}")
    version_file = package_root / "src" / "ratatoskr" / "_version.py"
    declared_version = runpy.run_path(str(version_file))["__version__"]
    if declared_version != arguments.version:
        raise RuntimeError(
            f"requested version {arguments.version} does not match Python source version "
            f"{declared_version}; update all product version files before release"
        )

    sources: list[tuple[Path, str, str, str]] = []
    for runtime, directory, filename, platform_tag in PLATFORMS:
        source = arguments.native_stage.resolve() / runtime / "native" / filename
        if not source.is_file():
            raise FileNotFoundError(f"missing release native library: {source}")
        sources.append((source, directory, filename, platform_tag))

    built_wheels: list[Path] = []
    try:
        for source, directory, filename, platform_tag in sources:
            _clear_build_state(package_root, native_root)
            destination = native_root / directory / filename
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
            before = set(output.glob("*.whl"))
            _run_build(
                package_root,
                output,
                [
                    "--wheel",
                    f"--config-setting=--build-option=--plat-name={platform_tag}",
                ],
            )
            created = set(output.glob("*.whl")) - before
            if len(created) != 1:
                raise RuntimeError(
                    f"expected one new wheel for {platform_tag}; got {created}"
                )
            wheel = created.pop()
            _validate_wheel(wheel, f"_native/{directory}/{filename}")
            built_wheels.append(wheel)

        _clear_build_state(package_root, native_root)
        _run_build(package_root, output, ["--sdist"])
        archives = list(output.glob("*.tar.gz"))
        if len(archives) != 1:
            raise RuntimeError(f"expected one source distribution; got {archives}")
        if len(built_wheels) != len(PLATFORMS):
            raise RuntimeError("not all platform wheels were built")
    finally:
        _clear_build_state(package_root, native_root)


if __name__ == "__main__":
    main()
