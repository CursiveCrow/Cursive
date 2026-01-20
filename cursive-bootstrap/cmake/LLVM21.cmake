# Find LLVM 21.1.8 in third_party
# This script sets up LLVM_INCLUDE_DIRS, LLVM_LIBRARY_DIRS, and LLVM_DEFINITIONS,
# and defines the necessary imported targets.

set(LLVM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third_party/llvm/llvm-21.1.8-x86_64")

if(EXISTS "${LLVM_ROOT}")
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
  # On Windows, we need to link against the .lib files in lib/
  link_directories("${LLVM_ROOT}/lib")

  if(MSVC)
    # LLVM binaries were built with VS2019 and hardcode path to diaguids.lib.
    # We are on VS2022, so we ignore the hardcoded path.
    # We use add_link_options to ensure it propagates to all targets.
    add_link_options("/NODEFAULTLIB:\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Professional\\DIA SDK\\lib\\amd64\\diaguids.lib\"")
    add_link_options("/NODEFAULTLIB:diaguids.lib")
    
    # And link the correct one found in the current environment
    if(EXISTS "C:/Program Files/Microsoft Visual Studio/2022/Community/DIA SDK/lib/amd64/diaguids.lib")
      link_libraries("C:/Program Files/Microsoft Visual Studio/2022/Community/DIA SDK/lib/amd64/diaguids.lib")
    else()
      message(WARNING "VS2022 diaguids.lib not found at expected path. Linker errors may occur.")
    endif()
  endif()


else()
  message(FATAL_ERROR "LLVM 21.1.8 not found in third_party/llvm/llvm-21.1.8-x86_64. Please ensure the dependency is present.")
endif()
