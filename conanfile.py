from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, cmake_layout, CMakeToolchain, CMakeDeps
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv


class MyProjectConan(ConanFile):
    name = "myproject"
    version = "0.1.0"
    package_type = "application"  # oder "library", je nachdem was du baust

    settings = "os", "arch", "compiler", "build_type"

    options = {"with_opencl": [True, False]}
    default_options = {"with_opencl": True}

    requires = (
        "opencl-icd-loader/2023.12.14",
        "freetype/2.13.2",
        "harfbuzz/11.4.1",
        "fontconfig/2.15.0",
        "libpng/1.6.43",
        "spdlog/1.14.1",
        "glm/1.0.1",
        "tinygltf/2.9.0",
        "cli11/2.6.0",
        "libgettext/0.22",
    )
#"sdl/3.2.20",
        
    # Für `conan create` nötig, sonst packst du nix ein:
    exports_sources = (
        "CMakeLists.txt",
        "src/*",
        "include/*",
        "cl/*",
        "shaders/*",
        "LICENSE*",
    )

    def build_requirements(self):
        self.tool_requires("cmake/3.29.6")
        self.tool_requires("ninja/1.12.1")
        #self.tool_requires("pkgconf/1.9.5")
    
    def layout(self):
        # Sauber: build/generators landen unter build/<build_type>/...
        cmake_layout(self, build_folder=f"build/{self.settings.build_type}")

    def validate(self):
        return
        # optional, aber nice: C++ Standard check
        # (nur wenn du wirklich z.B. C++23 brauchst)
        #if self.settings.compiler.get_safe("cppstd"):
        #    cppstd = int(str(self.settings.compiler.cppstd))
        #    if cppstd < 20:
        #        raise ConanInvalidConfiguration("Need at least C++20")

    def generate(self):
        CMakeDeps(self).generate()

        tc = CMakeToolchain(self)
        tc.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = True
        tc.cache_variables["CMAKE_INTERPROCEDURAL_OPTIMIZATION"] = False
        tc.generate()

        VirtualBuildEnv(self).generate()
        VirtualRunEnv(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        # Wenn dein CMakeLists ein install() hat → perfekt:
        cmake = CMake(self)
        cmake.install()
    


# TODO: durch alle repos und aarch abfragen