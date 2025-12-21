from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout

class MyProjectConan(ConanFile):
    name = "myproject"
    version = "0.1.0"

    settings = "os", "arch", "compiler", "build_type"

    # Deine externen Libraries hier rein:
    requires = (
        "opencl-icd-loader/2023.12.14",
        "freetype/2.13.2",
        "harfbuzz/11.4.1",
        "fontconfig/2.15.0",
        "libpng/1.6.43",
        "spdlog/1.14.1",
        "glm/1.0.1",
        "tinygltf/2.9.0",
        "sdl/3.2.20",
        "libgettext/0.22"
        
    )

    generators = ("CMakeDeps", "CMakeToolchain")

    def layout(self):
        cmake_layout(self)

    def build(self):
        print("Hallo :)")
        
        
        
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
