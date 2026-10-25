# metadata-util: reads and writes image metadata for DroneDB through Exiv2, in its own process.
#
# Builds standalone or as a subdirectory of DroneDB. It never links libddb: the two programs
# only exchange the text protocol of PROTOCOL.md.
cmake_minimum_required(VERSION 3.16)
project(metadata-util LANGUAGES CXX)

# Version printed by `metadata-util --version`: -DMETADATA_UTIL_VERSION=..., else the APP_VERSION of the
# DroneDB build that adds this directory, else "unknown".
set(METADATA_UTIL_VERSION "" CACHE STRING "Version reported by metadata-util --version")
if(METADATA_UTIL_VERSION STREQUAL "")
    if(DEFINED APP_VERSION AND NOT APP_VERSION STREQUAL "")
        set(METADATA_UTIL_VERSION_EFFECTIVE "${APP_VERSION}")
    else()
        set(METADATA_UTIL_VERSION_EFFECTIVE "unknown")
    endif()
else()
    set(METADATA_UTIL_VERSION_EFFECTIVE "${METADATA_UTIL_VERSION}")
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    endif()
    # Standalone: the executable lands in the build root, like it does in DroneDB's
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

find_package(Threads REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/version_info.h.in" "${GENERATED_DIR}/version_info.h" @ONLY)

add_executable(metadata-util
    src/main.cpp
    src/cli.cpp
    src/server.cpp
    src/service.cpp
    src/protocol.cpp
    src/json_writer.cpp
    src/utf8.cpp
    src/base64.cpp
    src/exiv2_image.cpp
    src/tag_serializer.cpp
    src/runtime.cpp
    src/parent_watchdog.cpp
)
target_include_directories(metadata-util PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${GENERATED_DIR}")
target_link_libraries(metadata-util PRIVATE Exiv2::exiv2lib nlohmann_json::nlohmann_json Threads::Threads)
set_target_properties(metadata-util PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
if(MSVC)
    target_compile_options(metadata-util PRIVATE /W4 /utf-8)
    # Exiv2 and nlohmann_json are the only users of these; keep the code warning-free anyway
    target_compile_definitions(metadata-util PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(metadata-util PRIVATE -Wall -Wextra)
endif()

install(TARGETS metadata-util RUNTIME DESTINATION bin)
