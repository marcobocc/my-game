file(GLOB_RECURSE EDITOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)
list(FILTER EDITOR_SOURCES EXCLUDE REGEX ".*Test\\.cpp$")
list(FILTER EDITOR_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(EditorLib STATIC ${EDITOR_SOURCES})
target_include_directories(EditorLib PUBLIC
    ${CMAKE_SOURCE_DIR}/runtime/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(EditorLib PUBLIC GameEngineLib nfd::nfd)
target_compile_definitions(EditorLib PUBLIC
    VK_NO_PROTOTYPES
    EDITOR_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

add_executable(editor ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
set_target_properties(editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(editor PRIVATE EditorLib)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    if (TEST_SOURCES)
        set(TEST_TARGET editorTests)
        add_executable(${TEST_TARGET} ${TEST_SOURCES})
        set_target_properties(${TEST_TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
        target_link_libraries(${TEST_TARGET} PRIVATE
            EditorLib
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
            GTest::gmock_main
        )
        target_compile_definitions(${TEST_TARGET} PRIVATE
            VK_NO_PROTOTYPES
            EDITOR_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
        )

        add_dependencies(editor ${TEST_TARGET})
        add_test(NAME ${TEST_TARGET} COMMAND ${CMAKE_BINARY_DIR}/bin/${TEST_TARGET})
    endif ()
endif ()
