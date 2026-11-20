include_guard(GLOBAL)

# Bison is a host tool. Homebrew keeps it outside PATH on macOS, where
# /usr/bin/bison is too old for the material grammar. Honor an explicit override.
if(CMAKE_HOST_APPLE AND NOT BISON_EXECUTABLE)
  find_program(DARK_HOMEBREW_EXECUTABLE NAMES brew
    HINTS /opt/homebrew/bin /usr/local/bin)
  mark_as_advanced(DARK_HOMEBREW_EXECUTABLE)

  set(dark_bison_hints
    /opt/homebrew/opt/bison/bin
    /usr/local/opt/bison/bin)
  if(DARK_HOMEBREW_EXECUTABLE)
    execute_process(
      COMMAND "${DARK_HOMEBREW_EXECUTABLE}" --prefix bison
      RESULT_VARIABLE dark_bison_prefix_result
      OUTPUT_VARIABLE dark_bison_prefix
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
      TIMEOUT 5)
    if(dark_bison_prefix_result STREQUAL "0" AND dark_bison_prefix)
      list(PREPEND dark_bison_hints "${dark_bison_prefix}/bin")
    endif()
  endif()

  find_program(BISON_EXECUTABLE NAMES bison
    HINTS ${dark_bison_hints} NO_DEFAULT_PATH)
endif()

find_package(BISON 3.0 REQUIRED)

# Generate a parser and its token header in the caller's build directory.
# Additional BISON_TARGET options, such as COMPILE_FLAGS, are forwarded unchanged.
# The generated file list is returned as <name>_sources.
function(dark_generate_parser name grammar source header)
  get_filename_component(grammar "${grammar}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  get_filename_component(source "${source}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  get_filename_component(header "${header}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  get_filename_component(source_directory "${source}" DIRECTORY)
  get_filename_component(header_directory "${header}" DIRECTORY)
  file(MAKE_DIRECTORY "${source_directory}" "${header_directory}")

  BISON_TARGET(${name} "${grammar}" "${source}"
    DEFINES_FILE "${header}" ${ARGN})
  set(${name}_sources ${BISON_${name}_OUTPUTS} PARENT_SCOPE)
endfunction()
