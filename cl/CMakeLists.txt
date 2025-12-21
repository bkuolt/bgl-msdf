cmake_minimum_required(VERSION 3.16)
project(libcl LANGUAGES CXX)

option(USE_CL "Enable OpenCL support" ON)
option(USE_PNG "Enable PNG support" ON)

#---------------------------------------
# set defines
if(USE_CL)
    add_definitions(-DUSE_CL)
endif()

if(USE_PNG)
    add_definitions(-DUSE_PNG)
endif()
# --------------------------------------

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCL REQUIRED)
find_package(spdlog REQUIRED)
find_package(PNG REQUIRED)

add_library(libcl OpenCl.cpp)
target_link_libraries(libcl PRIVATE 
    spdlog::spdlog 
    OpenCL::OpenCL
    PNG::PNG
) 