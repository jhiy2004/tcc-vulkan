cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(tcc)

add_executable(tcc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/window.cpp
)

target_include_directories(tcc
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Vulkan REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw-3.4)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb_image)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui-1.92.6)

target_link_libraries(tcc
    stb_image
    glm
    imgui
)