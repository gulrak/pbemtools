####################################################
#
#  FILE:
#  CommonSetup.cmake
#
#  BSD 3-Clause License
#  --------------------
#  
#  Copyright (c) 2015 Steffen Schümann, all rights reserved.
#  
#  1. Redistribution and use in source and binary forms, with or without
#     modification, are permitted provided that the following conditions are met:
#  
#  2. Redistributions of source code must retain the above copyright notice, this
#     list of conditions and the following disclaimer.
#  
#  3. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution. Neither the name of
#     the copyright holder nor the names of its contributors may be used to endorse
#     or promote products derived from this software without specific prior written
#     permission.
#  
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
#  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
#  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
###########################################################################

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

find_package(Git REQUIRED)

execute_process(
  COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_NUM
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
math(EXPR BUILD_NUMBER_EMU 469+${GIT_COMMIT_NUM})

string(TOLOWER ${PROJECT_NAME} PROJECT_LOWERCASE_NAME)
configure_file(${PROJECT_SOURCE_DIR}/version.h.in ${CMAKE_BINARY_DIR}/${PROJECT_LOWERCASE_NAME}/version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# make sure c++14 is used
message("Configure build files to use C++14")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(UNIX)
    if(CMAKE_CXX_INCLUDE_WHAT_YOU_USE) 
        set(COMMON_WARNINGS "-Wno-format")
    else()
        set(COMMON_WARNINGS "-Wall -Wextra -Wno-unknown-warning-option -Wshadow -Wmissing-include-dirs -Wfloat-equal -Wpointer-arith -Wunreachable-code -Wno-non-virtual-dtor -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-format-nonliteral -Wno-format -Wno-psabi -Werror")
    endif()
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(SYSTEM_LIBS dl)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_WARNINGS}")
        #set(CMAKE_CXX_FLAGS "-static ${CMAKE_CXX_FLAGS} ${COMMON_WARNINGS}")
        #set(CMAKE_C_FLAGS "-static ${CMAKE_C_FLAGS}")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        if(BUILD_32BIT)
            add_definitions(-m32)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
            set(CMAKE_SHARED_LIBRARY_C_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS} -m32")
            set(CMAKE_SHARED_LIBRARY_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CXX_FLAGS} -m32")
        endif()
        set(SYSTEM_LIBS dl pthread)
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            # we are using Clang under Linux
            set(CMAKE_CXX_FLAGS "-static ${CMAKE_CXX_FLAGS} ${COMMON_WARNINGS}")
            set(CMAKE_C_FLAGS "-static ${CMAKE_C_FLAGS}")
            list(APPEND SYSTEM_LIBS c++abi)
            message(STATUS "Selected clang...")
        elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            # we are using GCC under Linux
            set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++ ${CMAKE_CXX_FLAGS} ${COMMON_WARNINGS}")
            set(CMAKE_C_FLAGS "-static-libgcc ${CMAKE_C_FLAGS}")
            
            execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
            if(GCC_VERSION VERSION_LESS 4.7)
                message(FATAL_ERROR "To compile with GCC, a version of 4.7 or newer is needed!")
            endif()
        else()
            message(FATAL_ERROR "Sorry, I couldn't recognize the compiler, so I don't know how to configure C++11!")
        endif()
    endif()
endif(UNIX)
if(WIN32)
    set(SYSTEM_LIBS "")
endif()

if (MSVC)
  if (PBEMTOOLS_MSVC_STATIC_RUNTIME)
    # set all of our submodules to static runtime
    set(PCRE_MSVC_STATIC_RUNTIME ON)

    # In case we are building static libraries, link also the runtime library statically
    # so that MSVCR*.DLL is not required at runtime.
    # https://msdn.microsoft.com/en-us/library/2kzt1wy3.aspx
    # This is achieved by replacing msvc option /MD with /MT and /MDd with /MTd
    # https://gitlab.kitware.com/cmake/community/wikis/FAQ#how-can-i-build-my-msvc-application-with-a-static-runtime
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
      if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
  else()
    set(PCRE_USE_MSVC_STATIC_RUNTIME OFF)
  endif()
endif()

#include_directories(${PROJECT_SOURCE_DIR}/include)

