# Squirrel can be built from source or linked from an installed development package.
option(DARK_ENABLE_SQUIRREL
  "Build the native Squirrel 3.2 provider" ON)
set(DARK_SQUIRREL_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../darkengine-deps/squirrel" CACHE PATH
    "Path to an external Squirrel 3.2 source checkout")
if(ANDROID AND (NOT DARK_SQUIRREL_SOURCE OR
   NOT EXISTS "${DARK_SQUIRREL_SOURCE}/include/squirrel.h"))
  message(FATAL_ERROR
    "Android requires DARK_SQUIRREL_SOURCE to name an extracted Squirrel 3.2 source tree")
endif()
if(DARK_ENABLE_SQUIRREL)
  if(EXISTS "${DARK_SQUIRREL_SOURCE}/include/squirrel.h")
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
  else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
      pkg_check_modules(DARK_SQUIRREL_PC QUIET IMPORTED_TARGET libsquirrel)
    endif()
    if(TARGET PkgConfig::DARK_SQUIRREL_PC)
      add_library(dark_squirrel INTERFACE)
      target_link_libraries(dark_squirrel INTERFACE PkgConfig::DARK_SQUIRREL_PC)
    else()
      find_path(DARK_SQUIRREL_INCLUDE_DIR squirrel.h)
      find_library(DARK_SQUIRREL_LIBRARY NAMES squirrel)
      find_library(DARK_SQSTDLIB_LIBRARY NAMES sqstdlib)
      if(NOT DARK_SQUIRREL_INCLUDE_DIR OR NOT DARK_SQUIRREL_LIBRARY OR
         NOT DARK_SQSTDLIB_LIBRARY)
        message(FATAL_ERROR
          "Squirrel 3.2 source or installed development package was not found. "
          "Run tools/bootstrap-dependencies.sh game, set DARK_SQUIRREL_SOURCE, "
          "install the Squirrel development package, or explicitly set "
          "DARK_ENABLE_SQUIRREL=OFF for a reduced build.")
      endif()
      add_library(dark_squirrel INTERFACE)
      target_include_directories(dark_squirrel INTERFACE "${DARK_SQUIRREL_INCLUDE_DIR}")
      target_link_libraries(dark_squirrel INTERFACE
        "${DARK_SQUIRREL_LIBRARY}" "${DARK_SQSTDLIB_LIBRARY}")
    endif()
  endif()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_compile_definitions(dark_squirrel INTERFACE _SQ64)
  endif()
  target_compile_definitions(dark_script PRIVATE DARK_ENABLE_SQUIRREL=1)
  target_link_libraries(dark_script PRIVATE dark_squirrel)
endif()
