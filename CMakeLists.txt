cmake_minimum_required(VERSION 3.22)
project(GameEngine VERSION 0.1.0 LANGUAGES C CXX)

# -------------------------------------------------------------------------
# C++ standard
# -------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------------------------------------------------
# Find dependencies via vcpkg
# -------------------------------------------------------------------------
find_package(EXPAT REQUIRED)
find_package(log4cxx CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)

# -------------------------------------------------------------------------
# Sources
# -------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES src/*.cpp src/*.hpp)

# -------------------------------------------------------------------------
# Executable
# -------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES})

# -------------------------------------------------------------------------
# Include directories
# -------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# -------------------------------------------------------------------------
# Link libraries
# -------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
        log4cxx
        fmt::fmt
        glfw
        glm::glm
        volk::volk
        volk::volk_headers
        Vulkan::Headers
        GPUOpen::VulkanMemoryAllocator
)


# On macOS, link MoltenVK explicitly
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MOLTENVK_LIBRARY})
endif ()

# Ensure VK_NO_PROTOTYPES is defined globally for all translation units
target_compile_definitions(${PROJECT_NAME} PRIVATE VK_NO_PROTOTYPES)

# -------------------------------------------------------------------------
# Compiler options
# -------------------------------------------------------------------------
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif ()

# -------------------------------------------------------------------------
# Optional Vulkan macro
# -------------------------------------------------------------------------
add_definitions(-DVULKAN_ENGINE)