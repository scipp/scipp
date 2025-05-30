import os

from conans import CMake, ConanFile, tools

CMAKE_PROJECT_STR = """project(
    ${UNITS_CMAKE_PROJECT_NAME}
    LANGUAGES C CXX
    VERSION 0.13.1
)"""


class UnitsConan(ConanFile):
    name = "LLNL-Units"
    version = "0.13.1"
    license = "BSD-3"
    url = "https://github.com/llnl/units"
    homepage = "https://units.readthedocs.io"
    description = (
        "A run-time C++ library for working with units "
        "of measurement and conversions between them "
        "and with string representations of units "
        "and measurements"
    )
    topics = (
        "units",
        "dimensions",
        "quantities",
        "physical-units",
        "dimensional-analysis",
        "run-time",
    )
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "base_type": ["uint32_t", "uint64_t"],
        "namespace": "ANY",
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "base_type": "uint32_t",
        "namespace": None,
    }
    generators = "cmake"

    def source(self):
        git = tools.Git("units")
        git.clone("https://github.com/LLNL/units.git")
        git.checkout("v" + self.version)

        cmake_project_str = (
            CMAKE_PROJECT_STR.replace("\n", os.linesep)
            if self.settings.os == "Windows"
            else CMAKE_PROJECT_STR
        )

        tools.replace_in_file(
            "units/CMakeLists.txt",
            cmake_project_str,
            cmake_project_str
            + """
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()""",
        )

    def build(self):
        cmake = CMake(self)
        cmake.definitions["UNITS_ENABLE_TESTS"] = "OFF"
        cmake.definitions["UNITS_BASE_TYPE"] = self.options.base_type
        if self.options["shared"]:
            cmake.definitions["UNITS_BUILD_SHARED_LIBRARY"] = "ON"
            cmake.definitions["UNITS_BUILD_STATIC_LIBRARY"] = "OFF"
        cmake.configure(source_folder="units")
        cmake.build(target="units")

    def package(self):
        self.copy("*.hpp", dst="include/units", src="units/units")
        self.copy("*units.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["units"]
        self.cpp_info.defines = [f"UNITS_BASE_TYPE={self.options.base_type}"]
