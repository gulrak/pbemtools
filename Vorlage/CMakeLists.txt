SET(VORLAGE_SOURCES
    #crashdumphandler.cpp
    CRNE.cpp
    Metascript.cpp
    Zugvorlage.cpp
)

SET(VORLAGE_HEADERS
    #crashdumphandler.h
    CRNE.h
    Metascript.h
    Zugvorlage.h
)

add_executable(vorlage ${VORLAGE_SOURCES} ${VORLAGE_HEADERS} ${CMAKE_BINARY_DIR}/${PROJECT_LOWERCASE_NAME}/version.h)
target_link_libraries(vorlage ebase)
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    target_compile_definitions(vorlage PRIVATE _CRT_SECURE_NO_WARNINGS)
    #target_compile_options(vorlage PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    #target_compile_options(vorlage PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

