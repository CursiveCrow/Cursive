add_library(cursive0_rt STATIC
  src/mem.c
  src/rtc_stubs.c
  src/panic.c
  src/spec_trace.c
  src/context.c
  src/region.c
  src/heap.c
  src/string_bytes.c
  src/parallel.c
  src/filesystem.c
)

target_include_directories(cursive0_rt PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(cursive0_rt PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

set_target_properties(cursive0_rt PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED YES
)

if(MSVC)
  target_compile_options(cursive0_rt PRIVATE /GS- /Zl)
  set_property(TARGET cursive0_rt PROPERTY MSVC_RUNTIME_CHECKS "None")
endif()

set_target_properties(cursive0_rt PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set_target_properties(cursive0_rt PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_${cfg} ${CMAKE_CURRENT_SOURCE_DIR}
  )
endforeach()

if(MSVC)
  set(KERNEL32_LIB "kernel32.lib")
  set(VCRUNTIME_LIB "")
  if(CMAKE_AR)
    get_filename_component(MSVC_BIN_DIR "${CMAKE_AR}" DIRECTORY)
    get_filename_component(MSVC_BIN_PARENT "${MSVC_BIN_DIR}" DIRECTORY)
    get_filename_component(MSVC_BIN_ROOT "${MSVC_BIN_PARENT}" DIRECTORY)
    get_filename_component(MSVC_ROOT "${MSVC_BIN_ROOT}" DIRECTORY)
    set(VCRUNTIME_CANDIDATE "${MSVC_ROOT}/lib/x64/vcruntime.lib")
    if(EXISTS "${VCRUNTIME_CANDIDATE}")
      set(VCRUNTIME_LIB "${VCRUNTIME_CANDIDATE}")
    endif()
  endif()
  set(MERGED_RT "${CMAKE_CURRENT_BINARY_DIR}/cursive0_rt.merged.lib")
  set(SANITIZED_RT "${CMAKE_CURRENT_BINARY_DIR}/cursive0_rt.sanitized.lib")
  add_custom_command(TARGET cursive0_rt POST_BUILD
    COMMAND "${CMAKE_AR}" /NOLOGO /OUT:${SANITIZED_RT} /REMOVE:KERNEL32.dll
            "$<TARGET_FILE:cursive0_rt>"
    COMMAND "${CMAKE_AR}" /NOLOGO /OUT:${MERGED_RT}
            "${SANITIZED_RT}" "${KERNEL32_LIB}" ${VCRUNTIME_LIB}
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${MERGED_RT}" "$<TARGET_FILE:cursive0_rt>"
    COMMAND "${CMAKE_COMMAND}" -E rm -f "${SANITIZED_RT}" "${MERGED_RT}"
    COMMENT "Merging kernel32 + vcruntime into cursive0_rt.lib"
  )
endif()
