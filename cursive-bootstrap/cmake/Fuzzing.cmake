if (NOT CURSIVE_ENABLE_FUZZING)
  return()
endif()

set(CURSIVE_FUZZING_ENGINE "libfuzzer" CACHE STRING "Fuzzing engine (libfuzzer)")
set(CURSIVE_FUZZING_SANITIZERS "address,undefined" CACHE STRING "Sanitizers for fuzzing")

if (NOT CURSIVE_FUZZING_ENGINE STREQUAL "libfuzzer")
  message(WARNING "Unsupported fuzzing engine: ${CURSIVE_FUZZING_ENGINE}")
  return()
endif()

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(WARNING "Fuzzing requires Clang with libFuzzer; skipping fuzz targets")
  return()
endif()

set(CURSIVE_FUZZ_FLAGS "-fsanitize=fuzzer")
if (CURSIVE_FUZZING_SANITIZERS)
  set(CURSIVE_FUZZ_FLAGS "-fsanitize=fuzzer,${CURSIVE_FUZZING_SANITIZERS}")
endif()

function(cursive_add_fuzz_target target)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs SOURCES LIBS)
  cmake_parse_arguments(FUZZ "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT FUZZ_SOURCES)
    message(FATAL_ERROR "cursive_add_fuzz_target(${target}) missing SOURCES")
  endif()

  add_executable(${target} ${FUZZ_SOURCES})
  target_link_libraries(${target} PRIVATE ${FUZZ_LIBS})
  target_compile_features(${target} PRIVATE cxx_std_20)
  target_compile_options(${target} PRIVATE ${CURSIVE_FUZZ_FLAGS} -fno-omit-frame-pointer)
  target_link_options(${target} PRIVATE ${CURSIVE_FUZZ_FLAGS})

  if (WIN32 AND CURSIVE_USE_BUNDLED_ICU AND DEFINED CURSIVE_ICU_DLLS)
    foreach(dll ${CURSIVE_ICU_DLLS})
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${dll}"
          $<TARGET_FILE_DIR:${target}>
        COMMENT "Copying ICU DLL for fuzz target: ${dll}"
      )
    endforeach()
  endif()
endfunction()

cursive_add_fuzz_target(lexer_fuzz
  SOURCES tests/fuzz/lexer_fuzz.cpp
  LIBS cursive0_syntax
)

cursive_add_fuzz_target(parser_fuzz
  SOURCES tests/fuzz/parser_fuzz.cpp
  LIBS cursive0_syntax
)

cursive_add_fuzz_target(source_load_fuzz
  SOURCES tests/fuzz/source_load_fuzz.cpp
  LIBS cursive0_core
)
