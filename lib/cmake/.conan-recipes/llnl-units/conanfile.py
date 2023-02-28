import os

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake
from conan.tools.files import replace_in_file
from conan.tools.scm import Git

CMAKE_PROJECT_STR = """project(
    UNITS
    LANGUAGES C CXX
    VERSION 0.7.0
)"""


class UnitsConan(ConanFile):
    name = "llnl_units"
    version = "0.7.0"
    license = "BSD-3"
    url = "https://github.com/llnl/units"
    homepage = "https://units.readthedocs.io"
    description = ("A run-time C++ library for working with units "
                   "of measurement and conversions between them "
                   "and with string representations of units "
                   "and measurements")
    topics = ("units", "dimensions", "quantities", "physical-units",
              "dimensional-analysis", "run-time")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "base_type": ["uint32_t", "uint64_t"],
        "namespace": ["ANY"]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "base_type": "uint32_t",
        "namespace": None
    }
    generators = "cmake"

    def source(self):
        git = Git("units")
        git.clone("https://github.com/LLNL/units.git")
        git.checkout("v" + self.version)

        cmake_project_str = (CMAKE_PROJECT_STR.replace("\n", os.linesep)
                             if self.settings.os == "Windows" else CMAKE_PROJECT_STR)

        replace_in_file(
            "units/CMakeLists.txt", cmake_project_str, cmake_project_str + """
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()""")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.definitions["UNITS_ENABLE_TESTS"] = "OFF"
        tc.definitions["UNITS_BASE_TYPE"] = self.options.base_type
        units_namespace = self.options.get_safe("namespace")
        if units_namespace:
            tc.definitions["UNITS_NAMESPACE"] = units_namespace
        if self.options["shared"]:
            tc.definitions["UNITS_BUILD_SHARED_LIBRARY"] = "ON"
            tc.definitions["UNITS_BUILD_STATIC_LIBRARY"] = "OFF"
        # The library uses C++14, but we want to set the namespace
        # to llnl::units which requires C++17.
        tc.definitions["CMAKE_CXX_STANDARD"] = "17"
        tc.generate(source_folder="units")

    def build(self):
        cmake = CMake(self)
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
        units_namespace = self.options.get_safe("namespace")
        self.cpp_info.defines = [f"UNITS_BASE_TYPE={self.options.base_type}"]
        if units_namespace:
            self.cpp_info.defines.append(f"UNITS_NAMESPACE={units_namespace}")
