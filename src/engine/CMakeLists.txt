# Engine CMakeLists.txt

file(GLOB_RECURSE ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_library(GameEngineLib STATIC ${ENGINE_SOURCES})

target_include_directories(GameEngineLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(GameEngineLib PUBLIC
    log4cxx
    fmt::fmt
    glfw
    glm::glm
    volk::volk
    volk::volk_headers
    Vulkan::Headers
    GPUOpen::VulkanMemoryAllocator
    imgui
)

# On macOS, link MoltenVK explicitly
if (APPLE)
    target_link_libraries(GameEngineLib PUBLIC ${MOLTENVK_LIBRARY})
endif ()

# Ensure VK_NO_PROTOTYPES is defined globally for all translation units
target_compile_definitions(GameEngineLib PUBLIC VK_NO_PROTOTYPES)

# Library Compiler options
if (MSVC)
    target_compile_options(GameEngineLib PRIVATE /W4 /permissive-)
else ()
    target_compile_options(GameEngineLib PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif ()

