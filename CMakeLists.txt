cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(tcc)

add_executable(tcc
    src/main.cpp
    src/loader.cpp
    src/app.cpp
    src/glfw_window_opengl.cpp
    src/glfw_window_vulkan.cpp
    src/vulkan_renderer.cpp
    src/opengl_renderer.cpp
    src/util.cpp
    src/camera.cpp
)


target_include_directories(tcc
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)


add_subdirectory(thirdparty/glfw-3.4)
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/stb_image)
add_subdirectory(thirdparty/glad)


if(WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()

add_subdirectory(thirdparty/volk-1.4.350)

add_subdirectory(thirdparty/imgui-1.92.6)

target_link_libraries(tcc
    PRIVATE
        stb_image
        glm
        imgui
        glad
        OpenGL::GL
        volk
)


target_compile_definitions(tcc
    PRIVATE
        SHADERS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        FILES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/files"
        NOMINMAX
)