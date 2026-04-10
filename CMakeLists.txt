cmake_minimum_required(VERSION 3.22)
project(GameEngine VERSION 0.1.0 LANGUAGES C CXX)

# -------------------------------------------------------------------------
# C++ standard
# -------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
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
# Vendored dependencies
# -------------------------------------------------------------------------
add_subdirectory(vendor)

# -------------------------------------------------------------------------
# Sources
# -------------------------------------------------------------------------
file(GLOB_RECURSE ENGINE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

# -------------------------------------------------------------------------
# Game Engine Library
# -------------------------------------------------------------------------
add_library(GameEngineLib STATIC ${ENGINE_SOURCES})

# -------------------------------------------------------------------------
# Library Include directories
# -------------------------------------------------------------------------
target_include_directories(GameEngineLib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# -------------------------------------------------------------------------
# Library Link libraries
# -------------------------------------------------------------------------
target_link_libraries(GameEngineLib PUBLIC
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
    target_link_libraries(GameEngineLib PUBLIC ${MOLTENVK_LIBRARY})
endif ()

# Ensure VK_NO_PROTOTYPES is defined globally for all translation units
target_compile_definitions(GameEngineLib PUBLIC VK_NO_PROTOTYPES)

# -------------------------------------------------------------------------
# Library Compiler options
# -------------------------------------------------------------------------
if (MSVC)
    target_compile_options(GameEngineLib PRIVATE /W4 /permissive-)
else ()
    target_compile_options(GameEngineLib PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif ()


# -------------------------------------------------------------------------
# Automatically discover and build all tools
# -------------------------------------------------------------------------
file(GLOB TOOL_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src/tools/*")
foreach (TOOL_DIR ${TOOL_DIRS})
    if (IS_DIRECTORY ${TOOL_DIR})
        get_filename_component(TOOL_NAME ${TOOL_DIR} NAME)
        if (EXISTS "${TOOL_DIR}/main.cpp")
            file(GLOB_RECURSE TOOL_SOURCES "${TOOL_DIR}/*.cpp" "${TOOL_DIR}/*.hpp")
            add_executable(${TOOL_NAME} ${TOOL_SOURCES})
            target_link_libraries(${TOOL_NAME} PRIVATE GameEngineLib)
        endif ()
    endif ()
endforeach ()
