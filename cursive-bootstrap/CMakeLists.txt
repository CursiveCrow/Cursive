cmake_minimum_required(VERSION 3.20)

project(cursive LANGUAGES C CXX)

# Set output directories for built artifacts
# This ensures executables and libraries are placed in predictable locations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)

include(CTest)

# T-LLVM-001: Integration
include(cmake/LLVM21.cmake)


option(CURSIVE_USE_BUNDLED_ICU "Use bundled ICU 72.1 from third_party/icu" ON)
option(CURSIVE_ENABLE_FUZZING "Build fuzz targets (libFuzzer)" OFF)

set(CURSIVE_ICU_LIBS "")
if(CURSIVE_USE_BUNDLED_ICU)
  if(WIN32)
    # Windows: Use pre-built MSVC binaries from third_party/icu
    # Select directory based on target architecture
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
      set(ICU_WIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/icu/winarm64")
      set(ICU_ARCH_DESC "ARM64")
    else()
      set(ICU_WIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/icu/win64")
      set(ICU_ARCH_DESC "x64")
    endif()
    message(STATUS "Using ICU for ${ICU_ARCH_DESC} from: ${ICU_WIN_DIR}")
    set(ICU_INCLUDE_DIR "${ICU_WIN_DIR}/include")
    # ICU uses different directory names for ARM64 vs x64
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
      set(ICU_LIB_DIR "${ICU_WIN_DIR}/libARM64")
      set(ICU_BIN_DIR "${ICU_WIN_DIR}/binARM64")
    else()
      set(ICU_LIB_DIR "${ICU_WIN_DIR}/lib64")
      set(ICU_BIN_DIR "${ICU_WIN_DIR}/bin64")
    endif()

    set(ICU_WIN_PREBUILT TRUE)
    if(NOT EXISTS "${ICU_INCLUDE_DIR}/unicode/utypes.h")
      set(ICU_WIN_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/icuuc.lib")
      set(ICU_WIN_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/icuin.lib")
      set(ICU_WIN_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/icudt.lib")
      set(ICU_WIN_PREBUILT FALSE)
    endif()

    if(NOT ICU_WIN_PREBUILT)
      message(FATAL_ERROR "Bundled ICU Windows binaries for ${ICU_ARCH_DESC} not found. Run setup_third_party.ps1 -Arch ${ICU_ARCH_DESC} to download.")
    endif()

    add_library(ICU::uc SHARED IMPORTED)
    add_library(ICU::i18n SHARED IMPORTED)
    add_library(ICU::data SHARED IMPORTED)
    set_target_properties(ICU::uc PROPERTIES
      IMPORTED_LOCATION "${ICU_BIN_DIR}/icuuc72.dll"
      IMPORTED_IMPLIB "${ICU_LIB_DIR}/icuuc.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
    )
    set_target_properties(ICU::i18n PROPERTIES
      IMPORTED_LOCATION "${ICU_BIN_DIR}/icuin72.dll"
      IMPORTED_IMPLIB "${ICU_LIB_DIR}/icuin.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
    )
    set_target_properties(ICU::data PROPERTIES
      IMPORTED_LOCATION "${ICU_BIN_DIR}/icudt72.dll"
      IMPORTED_IMPLIB "${ICU_LIB_DIR}/icudt.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
    )
    set(CURSIVE_ICU_LIBS ICU::i18n ICU::uc ICU::data)

    # Copy DLLs to output directory for runtime
    set(CURSIVE_ICU_DLLS
      "${ICU_BIN_DIR}/icuuc72.dll"
      "${ICU_BIN_DIR}/icuin72.dll"
      "${ICU_BIN_DIR}/icudt72.dll"
    )
  else()
    # Unix/Linux: Use pre-built static libraries or build from source
    set(ICU_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/icu/install")
    set(ICU_INCLUDE_DIR "${ICU_INSTALL_DIR}/include")
    set(ICU_LIB_DIR "${ICU_INSTALL_DIR}/lib")
    set(ICU_PREBUILT TRUE)
    if(NOT EXISTS "${ICU_INCLUDE_DIR}/unicode/utypes.h")
      set(ICU_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/libicuuc.a")
      set(ICU_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/libicui18n.a")
      set(ICU_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/libicudata.a")
      set(ICU_PREBUILT FALSE)
    endif()

    if(ICU_PREBUILT)
      add_library(ICU::uc STATIC IMPORTED)
      add_library(ICU::i18n STATIC IMPORTED)
      add_library(ICU::data STATIC IMPORTED)
      set_target_properties(ICU::uc PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicuuc.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      set_target_properties(ICU::i18n PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicui18n.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      set_target_properties(ICU::data PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicudata.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      set(CURSIVE_ICU_LIBS ICU::i18n ICU::uc ICU::data)
    else()
      include(ExternalProject)
      set(ICU_TARBALL "${CMAKE_CURRENT_SOURCE_DIR}/third_party/icu/icu4c-72_1-src.tgz")
      if(NOT EXISTS "${ICU_TARBALL}")
        message(FATAL_ERROR "Missing bundled ICU tarball: ${ICU_TARBALL}")
      endif()
      file(MAKE_DIRECTORY "${ICU_INSTALL_DIR}/include")
      file(MAKE_DIRECTORY "${ICU_INSTALL_DIR}/lib")
      ExternalProject_Add(icu_external
        URL "${ICU_TARBALL}"
        SOURCE_SUBDIR "icu/source"
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND <SOURCE_DIR>/runConfigureICU Linux --disable-tests --disable-samples --disable-extras --disable-icuio --enable-static --disable-shared --with-data-packaging=static --prefix=${ICU_INSTALL_DIR}
        BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
        INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} install
      )

      add_library(ICU::uc STATIC IMPORTED)
      add_library(ICU::i18n STATIC IMPORTED)
      add_library(ICU::data STATIC IMPORTED)
      set_target_properties(ICU::uc PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicuuc.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      set_target_properties(ICU::i18n PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicui18n.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      set_target_properties(ICU::data PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicudata.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      add_dependencies(ICU::uc icu_external)
      add_dependencies(ICU::i18n icu_external)
      add_dependencies(ICU::data icu_external)
      set(CURSIVE_ICU_LIBS ICU::i18n ICU::uc ICU::data)
    endif()
  endif()
else()
  find_package(ICU REQUIRED COMPONENTS uc i18n)
  set(CURSIVE_ICU_LIBS ICU::uc ICU::i18n)
endif()

if(WIN32)
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "Windows builds require MSVC (cl.exe). Use a Visual Studio generator (e.g. \"Visual Studio 17 2022\") and toolset v143.")
  else()
    # Fix PDB locking issues with parallel builds
    add_compile_options(/FS)
  endif()
endif()

add_subdirectory(runtime)

add_subdirectory(src)

# Copy ICU DLLs to output directory on Windows
if(WIN32 AND CURSIVE_USE_BUNDLED_ICU AND DEFINED CURSIVE_ICU_DLLS)
  foreach(dll ${CURSIVE_ICU_DLLS})
    add_custom_command(TARGET cursivec0 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${dll}"
        $<TARGET_FILE_DIR:cursivec0>
      COMMENT "Copying ICU DLL: ${dll}"
    )
  endforeach()
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

if(CURSIVE_ENABLE_FUZZING)
  include(cmake/Fuzzing.cmake)
endif()

