# Android uses the OpenSL backend from the pinned OpenAL Soft source checkout.
if(ANDROID)
  if(NOT DARK_OPENAL_SOFT_SOURCE OR
     NOT EXISTS "${DARK_OPENAL_SOFT_SOURCE}/CMakeLists.txt")
    message(FATAL_ERROR
      "Android requires DARK_OPENAL_SOFT_SOURCE to name an extracted OpenAL Soft source tree")
  endif()
  set(ALSOFT_UTILS OFF CACHE BOOL "Do not build OpenAL Soft utilities" FORCE)
  set(ALSOFT_EXAMPLES OFF CACHE BOOL "Do not build OpenAL Soft examples" FORCE)
  set(ALSOFT_TESTS OFF CACHE BOOL "Do not build OpenAL Soft tests" FORCE)
  set(ALSOFT_INSTALL OFF CACHE BOOL "Do not create OpenAL Soft install rules" FORCE)
  set(ALSOFT_DLOPEN OFF CACHE BOOL "Link the Android OpenSL backend directly" FORCE)
  set(ALSOFT_BACKEND_OBOE OFF CACHE BOOL "Do not require the separate Oboe dependency" FORCE)
  set(ALSOFT_BACKEND_OPENSL ON CACHE BOOL "Use Android OpenSL for playback" FORCE)
  set(ALSOFT_REQUIRE_OPENSL ON CACHE BOOL "Require Android OpenSL support" FORCE)
  set(ALSOFT_RTKIT OFF CACHE BOOL "RTKit and D-Bus are not available on Android" FORCE)
  set(ALSOFT_BACKEND_WAVE OFF CACHE BOOL "Do not build the file writer backend" FORCE)
  add_subdirectory("${DARK_OPENAL_SOFT_SOURCE}"
    "${CMAKE_BINARY_DIR}/third_party/openal-soft" EXCLUDE_FROM_ALL)
endif()
