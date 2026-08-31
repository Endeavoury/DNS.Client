from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy
import os


class RatatoskrConan(ConanFile):
    name = "ratatoskr"
    version = "0.1.0"
    package_type = "library"
    license = "MIT"
    url = "https://github.com/Endeavoury/Ratatoskr"
    description = "Portable C networking core and C++ wrapper"
    topics = ("dns", "networking", "c", "c++")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": True, "fPIC": True}
    exports_sources = "CMakeLists.txt", "cmake/*", "include/*", "src/*", "bindings/cpp/include/*", "packaging/*", "LICENSE"

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        toolchain = CMakeToolchain(self)
        toolchain.variables["RATOS_BUILD_SHARED"] = bool(self.options.shared)
        toolchain.variables["RATOS_BUILD_STATIC"] = not bool(self.options.shared)
        toolchain.variables["RATOS_BUILD_CLI"] = False
        toolchain.variables["RATOS_BUILD_TESTS"] = False
        toolchain.generate()
        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "*.hpp", src=os.path.join(self.source_folder, "bindings", "cpp", "include"), dst=os.path.join(self.package_folder, "include"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "Ratatoskr")
        self.cpp_info.set_property("cmake_target_name", "Ratatoskr::ratatoskr")
        self.cpp_info.set_property("pkg_config_name", "ratatoskr")
        self.cpp_info.libs = ["ratatoskr"]
        if self.settings.os == "Windows":
            self.cpp_info.system_libs = ["ws2_32", "iphlpapi"]
