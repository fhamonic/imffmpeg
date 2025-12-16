from conan import ConanFile
from conan.tools.cmake import CMake
from conan.tools.files import copy


class ImFFMPEG(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    build_policy = "missing"

    def requirements(self):
        self.requires("boost/1.80.0")
        self.requires("nlohmann_json/3.11.2")

        self.requires("imgui/1.89.4")
        self.requires("glfw/3.3.8")
        self.requires("glew/2.2.0")

    def build_requirements(self):
        self.tool_requires("cmake/3.27.1")

    def generate(self):
        for lib, dep in self.dependencies.items():
            if lib.ref.name == "imgui":
                imgui_path = dep.package_folder
                break
        else:
            raise ValueError("imgui not found in dependencies")

        copy(
            self,
            "*.cpp",
            imgui_path + "/res/bindings",
            self.source_folder + "/imgui_backends",
        )
        copy(
            self,
            "*.h",
            imgui_path + "/res/bindings",
            self.source_folder + "/imgui_backends",
        )
        copy(
            self,
            "*.ttf",
            imgui_path + "/res/fonts",
            self.source_folder + "/imgui_fonts",
        )

        print("conanfile.py: IDE include dirs:")
        for lib, dep in self.dependencies.items():
            if not lib.headers:
                continue
            for inc in dep.cpp_info.includedirs:
                print("\t" + inc)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
