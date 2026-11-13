# Squirrel is built statically from the pinned source submodule on every
# platform.
option(DARK_ENABLE_SQUIRREL
  "Build the native Squirrel 3.2 provider" ON)
set(DARK_SQUIRREL_SOURCE
  "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/squirrel" CACHE PATH
  "Pinned Squirrel source submodule" FORCE)
if(DARK_ENABLE_SQUIRREL)
  if(NOT EXISTS "${DARK_SQUIRREL_SOURCE}/include/squirrel.h")
    message(FATAL_ERROR
      "Squirrel source is missing. Initialize the submodules with "
      "git submodule update --init --recursive.")
  endif()
  set(dark_squirrel_sources
    sqapi.cpp sqbaselib.cpp sqclass.cpp sqcompiler.cpp sqdebug.cpp
    sqfuncstate.cpp sqlexer.cpp sqmem.cpp sqobject.cpp sqstate.cpp
    sqtable.cpp sqvm.cpp)
  list(TRANSFORM dark_squirrel_sources
    PREPEND "${DARK_SQUIRREL_SOURCE}/squirrel/")
  set(dark_squirrel_stdlib_sources
    sqstdaux.cpp sqstdblob.cpp sqstdio.cpp sqstdmath.cpp sqstdrex.cpp
    sqstdstream.cpp sqstdstring.cpp sqstdsystem.cpp)
  list(TRANSFORM dark_squirrel_stdlib_sources
    PREPEND "${DARK_SQUIRREL_SOURCE}/sqstdlib/")
  add_library(dark_squirrel STATIC
    ${dark_squirrel_sources} ${dark_squirrel_stdlib_sources})
  target_include_directories(dark_squirrel PUBLIC
    "${DARK_SQUIRREL_SOURCE}/include")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_compile_definitions(dark_squirrel PUBLIC _SQ64)
  endif()
endif()
