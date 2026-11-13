# Instrument engine code without changing the ABI or build flags of dependencies.
set(DARK_SANITIZERS "" CACHE STRING
  "Engine sanitizers (semicolon-separated): address, undefined, thread, leak")
add_library(dark_sanitizers INTERFACE)

if(NOT DARK_SANITIZERS)
  return()
endif()

if(MSVC)
  if(NOT DARK_SANITIZERS STREQUAL "address")
    message(FATAL_ERROR "MSVC sanitizer builds support DARK_SANITIZERS=address only")
  endif()
  # CMake's default Debug runtime checks and incremental linking conflict
  # with MSVC AddressSanitizer. Keep these adjustments in this build tree.
  foreach(language C CXX OBJCXX)
    string(REGEX REPLACE "/RTC[^ ]*" "" CMAKE_${language}_FLAGS_DEBUG
      "${CMAKE_${language}_FLAGS_DEBUG}")
    string(REPLACE "/ZI" "/Zi" CMAKE_${language}_FLAGS_DEBUG
      "${CMAKE_${language}_FLAGS_DEBUG}")
  endforeach()
  target_compile_options(dark_sanitizers INTERFACE /fsanitize=address /Zi /Oy-)
  target_link_options(dark_sanitizers INTERFACE /INCREMENTAL:NO /DEBUG)
  return()
endif()

if(NOT CMAKE_C_COMPILER_ID MATCHES "^(Clang|AppleClang|GNU)$" OR
   NOT CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|AppleClang|GNU)$")
  message(FATAL_ERROR "DARK_SANITIZERS requires Clang, GCC, or MSVC")
endif()

set(dark_supported_sanitizers address undefined thread leak)
foreach(sanitizer IN LISTS DARK_SANITIZERS)
  if(NOT sanitizer IN_LIST dark_supported_sanitizers)
    message(FATAL_ERROR "Unsupported DARK_SANITIZERS entry: ${sanitizer}")
  endif()
endforeach()
if("thread" IN_LIST DARK_SANITIZERS AND
   ("address" IN_LIST DARK_SANITIZERS OR "leak" IN_LIST DARK_SANITIZERS))
  message(FATAL_ERROR "ThreadSanitizer must be used separately from address/leak sanitizers")
endif()
if(APPLE AND "leak" IN_LIST DARK_SANITIZERS)
  message(FATAL_ERROR "Standalone LeakSanitizer is not supported on macOS")
endif()

list(JOIN DARK_SANITIZERS "," dark_sanitizer_flags)
target_compile_options(dark_sanitizers INTERFACE
  "-fsanitize=${dark_sanitizer_flags}"
  -fno-omit-frame-pointer
  -fno-sanitize=alignment
  -fno-optimize-sibling-calls
  -g3
  -O0
)
target_link_options(dark_sanitizers INTERFACE
  "-fsanitize=${dark_sanitizer_flags}"
)
