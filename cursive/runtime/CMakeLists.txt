# =============================================================================
# Cursive Runtime Library
# =============================================================================
#
# This builds the Cursive runtime library (cursive0_rt) which provides:
# - Memory management (heap, region allocators)
# - String and bytes operations
# - Context and capability initialization
# - File system operations
# - Structured concurrency (spawn/wait, dispatch, cancellation)
# - Panic handling
#
# The runtime is written in pure C (not C++) for ABI stability.
# It is CRT-free, using Windows API directly.

get_filename_component(CURSIVE_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." REALPATH)
set(CURSIVE_EXTERN_DIR "${CURSIVE_REPO_ROOT}/extern")

if(WIN32)
  set(RUNTIME_ICU_ROOT "${CURSIVE_EXTERN_DIR}/icu/win64")
  set(RUNTIME_ICU_INCLUDE_DIR "${RUNTIME_ICU_ROOT}/include")
  set(RUNTIME_ICU_LIB_DIR "${RUNTIME_ICU_ROOT}/lib64")
  set(RUNTIME_ICU_BIN_DIR "${RUNTIME_ICU_ROOT}/bin64")
  set(RUNTIME_ICU_LIBS
    "${RUNTIME_ICU_LIB_DIR}/icuuc.lib"
    "${RUNTIME_ICU_LIB_DIR}/icuin.lib"
    "${RUNTIME_ICU_LIB_DIR}/icudt.lib"
  )
  set(RUNTIME_ICU_DLLS
    "${RUNTIME_ICU_BIN_DIR}/icuuc72.dll"
    "${RUNTIME_ICU_BIN_DIR}/icuin72.dll"
    "${RUNTIME_ICU_BIN_DIR}/icudt72.dll"
  )
else()
  set(RUNTIME_ICU_ROOT "${CURSIVE_EXTERN_DIR}/icu/install")
  set(RUNTIME_ICU_INCLUDE_DIR "${RUNTIME_ICU_ROOT}/include")
  set(RUNTIME_ICU_LIB_DIR "${RUNTIME_ICU_ROOT}/lib")
  set(RUNTIME_ICU_LIBS
    "${RUNTIME_ICU_LIB_DIR}/libicuuc.a"
    "${RUNTIME_ICU_LIB_DIR}/libicui18n.a"
    "${RUNTIME_ICU_LIB_DIR}/libicudata.a"
  )
endif()

add_library(cursive0_rt STATIC
  # Core
  src/core/mem.c
  src/core/panic.c
  src/core/spec_trace.c

  # Context
  src/context/context.c
  src/context/host_session.c
  src/context/heap.c

  # Memory
  src/memory/region.c
  src/memory/async.c
  src/memory/string_bytes.c

  # I/O
  src/io/filesystem.c
  src/network/network.c
  src/system/process.c
  src/system/raw_dylib.c

  # Concurrency
  src/concurrency/key.c
  src/concurrency/parallel.c

  # Compatibility
  src/compat/rtc_stubs.c
)

target_include_directories(cursive0_rt PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(cursive0_rt PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${RUNTIME_ICU_INCLUDE_DIR}
)

target_link_libraries(cursive0_rt PUBLIC
  ${RUNTIME_ICU_LIBS}
)

set_target_properties(cursive0_rt PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED YES
)

if(MSVC)
  # /GS- disables buffer security checks
  # /Zl omits default library name in .obj
  # /RTC- would disable runtime checks but we use stubs instead
  target_compile_options(cursive0_rt PRIVATE /GS- /Zl)
endif()

add_executable(cursive0_rt_task070_test
  tests/task070_region_runtime.c
)

target_link_libraries(cursive0_rt_task070_test PRIVATE
  cursive0_rt
)

target_include_directories(cursive0_rt_task070_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(cursive0_rt_task070_test PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED YES
)

if(WIN32)
  add_custom_command(TARGET cursive0_rt_task070_test POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${RUNTIME_ICU_DLLS}
            "$<TARGET_FILE_DIR:cursive0_rt_task070_test>"
    COMMENT "Copying ICU DLLs for runtime tests"
  )
endif()

add_executable(cursive0_rt_task075_test
  tests/task075_filesystem_runtime.c
)

target_link_libraries(cursive0_rt_task075_test PRIVATE
  cursive0_rt
)

target_include_directories(cursive0_rt_task075_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(cursive0_rt_task075_test PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED YES
)

if(WIN32)
  add_custom_command(TARGET cursive0_rt_task075_test POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${RUNTIME_ICU_DLLS}
            "$<TARGET_FILE_DIR:cursive0_rt_task075_test>"
    COMMENT "Copying ICU DLLs for runtime tests"
  )
endif()

add_executable(cursive0_rt_task104_test
  tests/task104_spec_trace_runtime.c
)

target_link_libraries(cursive0_rt_task104_test PRIVATE
  cursive0_rt
)

target_include_directories(cursive0_rt_task104_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(cursive0_rt_task104_test PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED YES
)

if(WIN32)
  add_custom_command(TARGET cursive0_rt_task104_test POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${RUNTIME_ICU_DLLS}
            "$<TARGET_FILE_DIR:cursive0_rt_task104_test>"
    COMMENT "Copying ICU DLLs for runtime tests"
  )
endif()

# On Windows, merge kernel32.lib into the runtime library so that executables
# linked with cursive0_rt don't need to explicitly link kernel32.lib.
#
# Note: vcruntime.lib is NOT merged because the runtime is CRT-free:
# - mem.c provides memset/memcpy/memmove/memcmp
# - rtc_stubs.c provides _RTC_* stubs
# - /GS- disables security cookies
# - Pure C code, no C++ exceptions or RTTI
# - libcmt.lib is merged only to provide __chkstk for large stack frames
#
# The library builds to the build directory, then gets merged/copy-staged into
# a stable runtime path under the build tree.
if(MSVC)
  set(FINAL_RT_DIR "${CMAKE_BINARY_DIR}/runtime")
  set(FINAL_RT "${FINAL_RT_DIR}/cursive0_rt.lib")
  add_custom_command(TARGET cursive0_rt POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${FINAL_RT_DIR}"
    COMMAND lib.exe /NOLOGO /OUT:${FINAL_RT}
            "$<TARGET_FILE:cursive0_rt>" "kernel32.lib" "libcmt.lib"
            ${RUNTIME_ICU_LIBS}
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${RUNTIME_ICU_DLLS}
            "${FINAL_RT_DIR}"
    COMMENT "Merging kernel32/libcmt into cursive0_rt.lib"
  )
else()
  # On non-Windows, stage the archive in the build runtime directory.
  set(FINAL_RT_DIR "${CMAKE_BINARY_DIR}/runtime")
  add_custom_command(TARGET cursive0_rt POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${FINAL_RT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:cursive0_rt>" "${FINAL_RT_DIR}/libcursive0_rt.a"
    COMMENT "Copying libcursive0_rt.a to build runtime directory"
  )
endif()
