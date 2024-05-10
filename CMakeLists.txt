# The Long Path patch for any x86/x64 .exe
# Copyright (c) 2012-2020 HALX99.
# Licensed under the MIT License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
# http://opensource.org/licenses/MIT
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT /O2" CACHE STRING "")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd /Od /Zi" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /O2 /Bv" CACHE STRING "")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /Od /Zi /Bv" CACHE STRING "")

project(wsls)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)  

set(CMAKE_CXX_STANDARD 17)

MARK_AS_ADVANCED(WSLS_PROJECT_DIR)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(CMAKE_INSTALL_BINDIR ${CMAKE_CURRENT_SOURCE_DIR}/dists)

add_definitions(-DUNICODE -D_UNICODE)
add_link_options("/Brepro")

# print all cmake builtin variables
# get_cmake_property(_variableNames VARIABLES)
# foreach (_variableName ${_variableNames})
#   message(STATUS "== ${_variableName}=${${_variableName}}")
# endforeach()

# minhook
set(MINHOOK_SRC minhook/src/trampoline.c
                       minhook/src/buffer.c
                       minhook/src/hook.c)
if(${MSVC_C_ARCHITECTURE_ID} STREQUAL "X86")
    set(MINHOOK_SRC ${MINHOOK_SRC}
                    minhook/src/hde/hde32.c)
    set(WSLS_ARCH "x86")
else()
    set(MINHOOK_SRC ${MINHOOK_SRC}
                    minhook/src/hde/hde64.c)
    set(WSLS_ARCH "x64")
endif()
add_library(minhook STATIC ${MINHOOK_SRC})
target_compile_definitions(minhook PRIVATE STRICT)
target_include_directories(minhook PUBLIC "minhook/include")

# libwsls
add_library(libwsls STATIC libwsls/libwsls.cpp)
target_include_directories(libwsls PUBLIC libwsls/)

# wsls-core
add_library(wsls-core SHARED wsls-core/wsls-core.cpp wsls-core/dllmain.cpp)
target_link_libraries(wsls-core libwsls minhook)
set_target_properties(wsls-core PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")

# wsls-shell
add_executable(wsls-shell wsls-shell/wsls-shell.cpp)
target_link_libraries(wsls-shell libwsls)

# wow64helper
add_executable(wow64helper wow64helper/wow64helper.cpp)
set_target_properties(wow64helper PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
# ---- commands

# wsls-copy
add_executable(wsls-copy wsls-copy/wsls-copy.cpp)
target_link_libraries(wsls-copy libwsls)
set_target_properties(wsls-copy
    PROPERTIES
    FOLDER "commands"
)

# wsls-del
add_executable(wsls-del wsls-del/wsls-del.cpp)
target_link_libraries(wsls-del libwsls)
set_target_properties(wsls-del
    PROPERTIES
    FOLDER "commands"
)

# wsls-echo
add_executable(wsls-echo wsls-echo/wsls-echo.cpp)
target_link_libraries(wsls-echo libwsls)
set_target_properties(wsls-echo
    PROPERTIES
    FOLDER "commands"
)

# wsls-md
add_executable(wsls-md wsls-md/wsls-md.cpp)
target_link_libraries(wsls-md libwsls)
set_target_properties(wsls-md
    PROPERTIES
    FOLDER "commands"
)

# ---- tools
add_executable(wsls-arch wsls-arch/wsls-arch.cpp)
set_target_properties(wsls-arch
    PROPERTIES
    FOLDER "tools"
)

add_executable(wsls-hash wsls-hash/wsls-hash.cpp wsls-hash/xxhash.c)
set_target_properties(wsls-hash
    PROPERTIES
    FOLDER "tools"
)

install(TARGETS wow64helper wsls-core wsls-shell wsls-copy wsls-del wsls-md wsls-echo
   DESTINATION ${CMAKE_INSTALL_BINDIR}/${WSLS_ARCH}
)

if(${MSVC_C_ARCHITECTURE_ID} STREQUAL "X86")
    install(TARGETS wsls-arch wsls-hash
        DESTINATION ${CMAKE_INSTALL_BINDIR}/
    )
endif()
