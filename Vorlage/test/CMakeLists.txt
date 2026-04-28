add_executable(metascript_catch_tests
    metascript_tests.cpp
)

target_link_libraries(metascript_catch_tests metascript Catch2::Catch2WithMain)
target_compile_definitions(metascript_catch_tests PRIVATE
    METASCRIPT_TEST_CONFIG_FILE="${CMAKE_SOURCE_DIR}/config/eressea.cfg"
)

if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    target_compile_definitions(metascript_catch_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(metascript_catch_tests PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    target_compile_options(metascript_catch_tests PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

if(APPLE)
    target_compile_options(metascript_catch_tests PRIVATE -mmacosx-version-min=10.15)
    target_link_options(metascript_catch_tests PRIVATE -mmacosx-version-min=10.15)
endif()

include(Catch)
catch_discover_tests(metascript_catch_tests)
