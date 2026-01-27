# Find LLVM 21.1.8 in third_party
# This script sets up LLVM_INCLUDE_DIRS, LLVM_LIBRARY_DIRS, and LLVM_DEFINITIONS,
# and defines the necessary imported targets.
#
# Supports both x64 and ARM64 architectures.

# Determine LLVM directory based on target architecture
if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
  set(LLVM_ARCH_DIR "llvm-21.1.8-aarch64")
  set(LLVM_DIA_ARCH "arm64")
else()
  set(LLVM_ARCH_DIR "llvm-21.1.8-x86_64")
  set(LLVM_DIA_ARCH "amd64")
endif()

set(LLVM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third_party/llvm/${LLVM_ARCH_DIR}")

if(EXISTS "${LLVM_ROOT}")
  message(STATUS "Using LLVM from: ${LLVM_ROOT}")
  
  # Point CMake to the config file
  set(LLVM_DIR "${LLVM_ROOT}/lib/cmake/llvm")
  
  find_package(LLVM REQUIRED CONFIG)
  
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
  message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
  
  # Include headers
  include_directories(${LLVM_INCLUDE_DIRS})
  
  # Definitions (e.g. -D__STDC_LIMIT_MACROS, -D__STDC_CONSTANT_MACROS)
  add_definitions(${LLVM_DEFINITIONS})
  
  # No rtti usually for LLVM
  if(NOT ${LLVM_ENABLE_RTTI})
     if(MSVC)
        add_compile_options(/GR-)
     else()
        add_compile_options(-fno-rtti)
     endif()
  endif()
  
  # Use llvm_map_components_to_libnames to get the library list.
  # We need: Core, IRReader, CodeGen, Support, Target, Linker, Analysis, Passes
  llvm_map_components_to_libnames(llvm_libs 
      core
      irreader 
      codegen 
      target 
      native
      linker 
      analysis 
      passes
      support
      # Add more components as needed
  )
  
  # On Windows, we need to link against the .lib files in lib/
  link_directories("${LLVM_ROOT}/lib")

  if(MSVC)
    # LLVM binaries were built with VS2019 and hardcode path to diaguids.lib.
    # We are on VS2022, so we ignore the hardcoded path.
    # We use add_link_options to ensure it propagates to all targets.
    add_link_options("/NODEFAULTLIB:\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Professional\\DIA SDK\\lib\\amd64\\diaguids.lib\"")
    add_link_options("/NODEFAULTLIB:diaguids.lib")

    # Find DIA SDK for the correct architecture
    # Try VS 2026 Build Tools first, then VS 2022 editions
    set(DIA_SDK_PATHS
      "C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/DIA SDK/lib/${LLVM_DIA_ARCH}/diaguids.lib"
      "C:/Program Files/Microsoft Visual Studio/2026/Community/DIA SDK/lib/${LLVM_DIA_ARCH}/diaguids.lib"
      "C:/Program Files/Microsoft Visual Studio/2026/Enterprise/DIA SDK/lib/${LLVM_DIA_ARCH}/diaguids.lib"
      "C:/Program Files/Microsoft Visual Studio/2026/Professional/DIA SDK/lib/${LLVM_DIA_ARCH}/diaguids.lib"
      "C:/Program Files/Microsoft Visual Studio/2022/Community/DIA SDK/lib/${LLVM_DIA_ARCH}/diaguids.lib"
      "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/DIA SDK/lib/${LLVM_DIA_ARCH}/diaguids.lib"
      "C:/Program Files/Microsoft Visual Studio/2022/Professional/DIA SDK/lib/${LLVM_DIA_ARCH}/diaguids.lib"
    )
    set(DIA_SDK_FOUND FALSE)
    foreach(DIA_SDK_PATH ${DIA_SDK_PATHS})
      if(EXISTS "${DIA_SDK_PATH}")
        link_libraries("${DIA_SDK_PATH}")
        message(STATUS "Found DIA SDK: ${DIA_SDK_PATH}")
        set(DIA_SDK_FOUND TRUE)
        break()
      endif()
    endforeach()
    if(NOT DIA_SDK_FOUND)
      message(WARNING "diaguids.lib not found for ${LLVM_DIA_ARCH}. Linker errors may occur.")
    endif()
  endif()

else()
  message(FATAL_ERROR "LLVM 21.1.8 not found in third_party/llvm/${LLVM_ARCH_DIR}. Please run setup_third_party.ps1 with the correct -Arch parameter.")
endif()
