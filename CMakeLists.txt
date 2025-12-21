cmake_minimum_required(VERSION 3.16)
project(ft_hello LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



find_package(OpenCL REQUIRED)

find_package(Freetype REQUIRED)
find_package(spdlog REQUIRED)

find_package(PNG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
pkg_check_modules(HARFBUZZ REQUIRED IMPORTED_TARGET harfbuzz)


add_executable(ft_hello main.cpp OpenCl.cpp)
target_link_libraries(ft_hello PRIVATE Freetype::Freetype spdlog::spdlog  PkgConfig::FONTCONFIG PkgConfig::HARFBUZZ OpenCL::OpenCL PNG::PNG) 

