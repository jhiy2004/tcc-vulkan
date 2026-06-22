add_library(imgui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(imgui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

target_compile_definitions(imgui
    PRIVATE
        IMGUI_IMPL_VULKAN_USE_VOLK
)

target_link_libraries(imgui
    PUBLIC glfw
    PUBLIC Vulkan::Vulkan
    PUBLIC volk
)