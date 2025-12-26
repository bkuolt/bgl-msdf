cmake_minimum_required(VERSION 3.16)
project(libfont LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_FREETYPE "Enable FreeType support" ON)
option(USE_FONTCONFIG "Enable Fontconfig support" ON)
option(USE_HARFBUZZ "Enable HarfBuzz support" ON)


find_package(spdlog REQUIRED)
find_package(PNG REQUIRED)
find_package(Freetype REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
pkg_check_modules(HARFBUZZ REQUIRED IMPORTED_TARGET harfbuzz)

add_library(libfont font.cpp render.cpp)
target_link_libraries(libfont PRIVATE 
    spdlog::spdlog 
    Freetype::Freetype 
    PkgConfig::FONTCONFIG
    PkgConfig::HARFBUZZ
    PNG::PNG
) 
