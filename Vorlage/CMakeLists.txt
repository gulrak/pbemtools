SET(METASCRIPT_SOURCES
    CRNE.cpp
    Metascript.cpp
)

SET(METASCRIPT_HEADERS
    CRNE.h
    Metascript.h
)

add_library(metascript STATIC ${METASCRIPT_SOURCES} ${METASCRIPT_HEADERS})
target_link_libraries(metascript PUBLIC ebase)
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    target_compile_definitions(metascript PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

SET(VORLAGE_SOURCES
    #crashdumphandler.cpp
    Zugvorlage.cpp
)

SET(VORLAGE_HEADERS
    #crashdumphandler.h
    CRNE.h
    Metascript.h
    Zugvorlage.h
)

add_executable(vorlage ${VORLAGE_SOURCES} ${VORLAGE_HEADERS} ${CMAKE_BINARY_DIR}/${PROJECT_LOWERCASE_NAME}/version.h)
target_link_libraries(vorlage metascript)
if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    target_compile_definitions(vorlage PRIVATE _CRT_SECURE_NO_WARNINGS)
    #target_compile_options(vorlage PRIVATE "$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    #target_compile_options(vorlage PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

add_subdirectory(test)

if(APPLE AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET vorlage POST_BUILD
        COMMAND codesign -f -s - --entitlements ${CMAKE_SOURCE_DIR}/cmake/debug.entitlements $<TARGET_FILE:vorlage>
        COMMENT "Signing vorlage with debug entitlements"
    )
endif()
