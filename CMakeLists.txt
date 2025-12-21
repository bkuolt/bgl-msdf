cmake_minimum_required(VERSION 3.16)
project(ft_hello LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(spdlog REQUIRED)

add_executable(ft_hello main.cpp)
add_subdirectory("cl")
add_subdirectory("font")

target_link_libraries(ft_hello PRIVATE 
    libcl libfont
    spdlog::spdlog 
)


add_custom_command(TARGET ft_hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:ft_hello>/assets
)
