"""Small wheel-tag customization for wheels containing the native C core."""

from __future__ import annotations

from setuptools import Distribution, setup
from setuptools.command.bdist_wheel import bdist_wheel


class BinaryDistribution(Distribution):
    """Install the package in platlib even though Python itself is pure."""

    def has_ext_modules(self) -> bool:
        return True


class PlatformWheel(bdist_wheel):
    """The wrapper is Python-ABI neutral, while its bundled C library is not."""

    def finalize_options(self) -> None:
        super().finalize_options()
        self.root_is_pure = False

    def get_tag(self) -> tuple[str, str, str]:
        _, _, platform_tag = super().get_tag()
        return "py3", "none", platform_tag


settings: dict[str, object] = {
    "distclass": BinaryDistribution,
    "cmdclass": {"bdist_wheel": PlatformWheel},
}
setup(**settings)
