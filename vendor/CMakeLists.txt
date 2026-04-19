set(VENDOR_DIR ${CMAKE_CURRENT_LIST_DIR})

# -------------------------------------------------------------------------
# ImGui
# -------------------------------------------------------------------------
set(IMGUI_DIR ${VENDOR_DIR}/imgui)
file(GLOB IMGUI_SOURCES
        ${IMGUI_DIR}/*.h
        ${IMGUI_DIR}/*.cpp
        # List of enabled imgui backends
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.h
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.h
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends $<TARGET_PROPERTY:glfw,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK)     # DO NOT REMOVE! (see details below)
# IMGUI_IMPL_VULKAN_USE_VOLK ensures that ImGui uses the Vulkan function pointers (vkGetInstanceProcAddr,
# vkGetDeviceProcAddr, etc) that are loaded via Volk. Removing IMGUI_IMPL_VULKAN_USE_VOLK will immediately result
# in a segmentation fault when initializing ImGui.

# -------------------------------------------------------------------------
# stb
# -------------------------------------------------------------------------
set(STB_DIR ${VENDOR_DIR}/stb)
set(STB_SOURCES ${STB_DIR}/stb_image_impl.cpp)
add_library(stb STATIC ${STB_SOURCES})
target_include_directories(stb PUBLIC ${STB_DIR})
