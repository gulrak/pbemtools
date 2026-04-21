set(EBASE_SOURCES
    charencoding.cpp
    #expr.cpp
    Expression.cpp
    Hash.cpp
    hierarchy.cpp
    regexp.cpp
    Report.cpp
    ReportBase.cpp
    ReportStream.cpp
    Utility.cpp
    Value.cpp
)

set(EBASE_HEADERS
    charencoding.h
    #expr.hpp
    Expression.h
    Hash.h
    hierarchy.h
    regexp.h
    Report.h
    ReportBase.h
    ReportStream.h
    Utility.h
    Value.h
    utf8.hpp
)

add_library(ebase STATIC ${EBASE_SOURCES} ${EBASE_HEADERS})
target_link_libraries(ebase PUBLIC pcre2-8 fmt::fmt)
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    target_compile_definitions(ebase PRIVATE _CRT_SECURE_NO_WARNINGS)
    #target_compile_options(ebase PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    #target_compile_options(ebase PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

add_subdirectory(test)

