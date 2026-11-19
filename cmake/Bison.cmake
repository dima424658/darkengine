include_guard(GLOBAL)

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
