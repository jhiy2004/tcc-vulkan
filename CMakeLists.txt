cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(tcc)

add_executable(tcc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glfw_window_opengl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glfw_window_vulkan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/opengl_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util.cpp
)

target_include_directories(tcc
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)

# Start Vulkan-Hpp configuration
add_library(VulkanCppModule)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

target_compile_definitions(VulkanCppModule PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

target_include_directories(VulkanCppModule PRIVATE "${Vulkan_INCLUDE_DIR}")

target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)

set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

target_sources(VulkanCppModule
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS "${Vulkan_INCLUDE_DIR}"
        FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
)
# End Vulkan-Hpp configuration


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glfw-3.4)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glm)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb_image)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui-1.92.6)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glad)

target_link_libraries(tcc
    stb_image
    glm
    imgui
    VulkanCppModule
    glad
    OpenGL::GL
)

target_compile_definitions(tcc
    PRIVATE SHADERS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)
