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

# -------------------------------------------------------------------------
# spirv-reflect
# -------------------------------------------------------------------------
set(SPIRV_REFLECT_DIR ${VENDOR_DIR}/spirv_reflect)
set(SPIRV_REFLECT_SOURCES ${SPIRV_REFLECT_DIR}/spirv_reflect.h ${SPIRV_REFLECT_DIR}/spirv_reflect.cpp)
add_library(spirv_reflect STATIC ${SPIRV_REFLECT_SOURCES})
target_include_directories(spirv_reflect PUBLIC ${SPIRV_REFLECT_DIR})

# -------------------------------------------------------------------------
# argparse
# -------------------------------------------------------------------------
set(ARGPARSE_DIR ${VENDOR_DIR}/argparse)
add_library(argparse INTERFACE)
target_include_directories(argparse INTERFACE ${VENDOR_DIR})

# -------------------------------------------------------------------------
# subprocess
# -------------------------------------------------------------------------
add_library(subprocess INTERFACE)
target_link_libraries(subprocess INTERFACE Threads::Threads)
target_sources(subprocess PUBLIC
        FILE_SET HEADERS
        FILES
        subprocess/subprocess.hpp
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/
)
add_library(cpp-subprocess::subprocess ALIAS subprocess)

# -------------------------------------------------------------------------
# Tracy Profiler
# -------------------------------------------------------------------------
option(TRACY_ENABLE "Enable Tracy profiler" OFF)
option(TRACY_ON_DEMAND "Enable Tracy on-demand profiling" ON)
add_subdirectory(${VENDOR_DIR}/tracy)

# -------------------------------------------------------------------------
# LuaJIT
# -------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(LUAJIT REQUIRED luajit)

add_library(luajit_lib INTERFACE)
target_include_directories(luajit_lib INTERFACE ${LUAJIT_INCLUDE_DIRS})
target_link_directories(luajit_lib INTERFACE ${LUAJIT_LIBRARY_DIRS})
target_link_libraries(luajit_lib INTERFACE ${LUAJIT_LIBRARIES})

# -------------------------------------------------------------------------
# sol2 (Lua bindings)
# -------------------------------------------------------------------------
set(SOL2_DIR ${VENDOR_DIR}/sol2)
add_library(sol2 INTERFACE)
target_include_directories(sol2 INTERFACE ${SOL2_DIR}/include)
target_link_libraries(sol2 INTERFACE luajit_lib)
target_compile_definitions(sol2 INTERFACE SOL_USE_LUAJIT=1 SOL_ALL_SAFETIES_ON=1)
