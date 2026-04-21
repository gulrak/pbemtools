set(VPP_SOURCES
    VPP.cpp
)

set(VPP_HEADERS
    StdAfx.h
)

add_executable(vpp ${VPP_SOURCES} ${VPP_HEADER} ${CMAKE_BINARY_DIR}/${PROJECT_LOWERCASE_NAME}/version.h)
target_link_libraries(vpp ebase pcre)
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    target_compile_definitions(vpp PRIVATE _CRT_SECURE_NO_WARNINGS)
    #target_compile_options(vpp PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    #target_compile_options(vpp PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

