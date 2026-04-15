cmake_minimum_required(VERSION 4.0.1)
project(cursive LANGUAGES C CXX)

set(CURSIVE_BUILD_DIR "${CMAKE_BINARY_DIR}")
file(MAKE_DIRECTORY "${CURSIVE_BUILD_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CURSIVE_BUILD_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CURSIVE_BUILD_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CURSIVE_BUILD_DIR}")

if(CMAKE_CONFIGURATION_TYPES)
  foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${cfg}" cfg_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg_upper} "${CURSIVE_BUILD_DIR}/${cfg}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg_upper} "${CURSIVE_BUILD_DIR}/${cfg}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfg_upper} "${CURSIVE_BUILD_DIR}/${cfg}")
  endforeach()
endif()

add_subdirectory(src)
add_subdirectory(runtime)
