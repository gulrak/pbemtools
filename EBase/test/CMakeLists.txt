
add_executable(ebase_catch_tests ctest_main.cpp expression_tests.cpp value_tests.cpp charencoding_test.cpp)
target_link_libraries(ebase_catch_tests ebase Catch2::Catch2WithMain)

if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    target_compile_definitions(ebase_catch_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(ebase_catch_tests PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    target_compile_options(ebase_catch_tests PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

include(Catch)
catch_discover_tests(ebase_catch_tests)
