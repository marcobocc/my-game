# Tools CMakeLists.txt

# Automatically discover and build all tools
file(GLOB TOOL_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach (TOOL_DIR ${TOOL_DIRS})
    if (IS_DIRECTORY ${TOOL_DIR})
        get_filename_component(TOOL_NAME ${TOOL_DIR} NAME)
        if (EXISTS "${TOOL_DIR}/main.cpp")
            file(GLOB_RECURSE TOOL_SOURCES "${TOOL_DIR}/*.cpp" "${TOOL_DIR}/*.hpp")
            add_executable(${TOOL_NAME} ${TOOL_SOURCES})
            set_target_properties(${TOOL_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
            target_link_libraries(${TOOL_NAME} PRIVATE GameEngineLib)
        endif ()
    endif ()
endforeach ()

