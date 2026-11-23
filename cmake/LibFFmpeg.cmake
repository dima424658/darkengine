set(dark_ffmpeg_components avformat avcodec swresample swscale avutil)
add_library(dark_ffmpeg INTERFACE)

if(WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(dark_ffmpeg_arch x64)
    set(dark_ffmpeg_hash fd34c286a9850583117f12f9ffa393e0a6e49708c7185f3db2c71f82ff134e0a)
  else()
    set(dark_ffmpeg_arch x86)
    set(dark_ffmpeg_hash 1d45c38e88e7c50410442fa32482ea4c4e06727b121d3e5788ef7a94a009d267)
  endif()

  set(dark_ffmpeg_version "8.1.3")
  set(dark_ffmpeg_release "ffmpeg-${dark_ffmpeg_version}")
  set(dark_ffmpeg_asset "${dark_ffmpeg_release}_${dark_ffmpeg_arch}-windows-shared-lgpl.zip")

  include(FetchContent)
  FetchContent_Declare(dark_ffmpeg_windows
    URL "https://github.com/System233/ffmpeg-msvc-prebuilt/releases/download/${dark_ffmpeg_release}/${dark_ffmpeg_asset}"
    URL_HASH "SHA256=${dark_ffmpeg_hash}"
    TLS_VERIFY TRUE
    SOURCE_SUBDIR dark-no-cmake-project)
  FetchContent_MakeAvailable(dark_ffmpeg_windows)

  # Some SDK archives include a versioned directory around the SDK.
  file(GLOB dark_ffmpeg_sdk_dirs "${dark_ffmpeg_windows_SOURCE_DIR}/ffmpeg-*")
  find_path(dark_ffmpeg_include_dir libavformat/avformat.h
    PATHS "${dark_ffmpeg_windows_SOURCE_DIR}" ${dark_ffmpeg_sdk_dirs}
    PATH_SUFFIXES include NO_CACHE NO_DEFAULT_PATH REQUIRED)
  get_filename_component(dark_ffmpeg_root "${dark_ffmpeg_include_dir}" DIRECTORY)

  set(dark_ffmpeg_libraries)
  foreach(component IN LISTS dark_ffmpeg_components)
    find_library(dark_ffmpeg_${component}_library
      NAMES "${component}"
      PATHS "${dark_ffmpeg_root}/lib" NO_CACHE NO_DEFAULT_PATH REQUIRED)
    list(APPEND dark_ffmpeg_libraries "${dark_ffmpeg_${component}_library}")
  endforeach()

  target_include_directories(dark_ffmpeg SYSTEM INTERFACE "${dark_ffmpeg_include_dir}")
  target_link_libraries(dark_ffmpeg INTERFACE
    ${dark_ffmpeg_libraries} bcrypt secur32 ws2_32 user32 advapi32)
  file(GLOB DARK_FFMPEG_RUNTIME_DLLS CONFIGURE_DEPENDS "${dark_ffmpeg_root}/bin/*.dll")
  return()
endif()

# Build the pinned FFmpeg source for the active CMake target. FFmpeg's own
# configure/make build is used because upstream does not provide CMake files.
include(ExternalProject)

set(DARK_FFMPEG_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/FFmpeg")
if(NOT EXISTS "${DARK_FFMPEG_SOURCE}/configure")
  message(FATAL_ERROR
    "FFmpeg source is missing. Initialize submodules with "
    "git submodule update --init --recursive.")
endif()

find_program(DARK_FFMPEG_MAKE NAMES gmake make)
if(ANDROID)
  find_program(DARK_FFMPEG_BASH NAMES bash)
endif()
if(NOT DARK_FFMPEG_MAKE OR (ANDROID AND NOT DARK_FFMPEG_BASH))
  message(FATAL_ERROR
    "The FFmpeg source build requires GNU Make and requires Bash on Android.")
endif()

set(DARK_FFMPEG_PREFIX "${CMAKE_BINARY_DIR}/third_party/ffmpeg-install")
set(DARK_FFMPEG_BINARY "${CMAKE_BINARY_DIR}/third_party/ffmpeg-build")
set(dark_ffmpeg_configure_args
  "--prefix=${DARK_FFMPEG_PREFIX}"
  --disable-shared
  --enable-static
  --disable-programs
  --disable-avdevice
  --disable-avfilter
  --disable-doc
  --disable-debug
  --disable-autodetect
  --disable-network
  --enable-pic
  --disable-x86asm
)

if(ANDROID)
  if(NOT CMAKE_ANDROID_NDK OR NOT CMAKE_SYSROOT)
    message(FATAL_ERROR
      "The FFmpeg Android build requires the Android NDK CMake toolchain.")
  endif()
  if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    set(dark_ffmpeg_arch aarch64)
    set(dark_ffmpeg_android_triple aarch64-linux-android)
  elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
    set(dark_ffmpeg_arch arm)
    set(dark_ffmpeg_android_triple armv7a-linux-androideabi)
  elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
    set(dark_ffmpeg_arch x86_64)
    set(dark_ffmpeg_android_triple x86_64-linux-android)
  elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
    set(dark_ffmpeg_arch x86)
    set(dark_ffmpeg_android_triple i686-linux-android)
  else()
    message(FATAL_ERROR
      "Unsupported Android ABI for FFmpeg: ${CMAKE_ANDROID_ARCH_ABI}")
  endif()
  set(dark_ffmpeg_compiler_target "${CMAKE_C_COMPILER_TARGET}")
  if(NOT dark_ffmpeg_compiler_target)
    set(dark_ffmpeg_compiler_target "${dark_ffmpeg_android_triple}${CMAKE_SYSTEM_VERSION}")
  endif()
  list(APPEND dark_ffmpeg_configure_args
    --target-os=android
    "--arch=${dark_ffmpeg_arch}"
    --enable-cross-compile
    "--cc=${CMAKE_C_COMPILER}"
    "--ar=${CMAKE_AR}"
    "--ranlib=${CMAKE_RANLIB}"
    "--nm=${CMAKE_NM}"
    "--strip=${CMAKE_STRIP}"
    "--sysroot=${CMAKE_SYSROOT}"
    "--extra-cflags=--target=${dark_ffmpeg_compiler_target} -fvisibility=hidden"
    "--extra-ldflags=--target=${dark_ffmpeg_compiler_target}")
endif()

set(dark_ffmpeg_configure_command "${DARK_FFMPEG_SOURCE}/configure")
if(DARK_FFMPEG_BASH)
  list(PREPEND dark_ffmpeg_configure_command "${DARK_FFMPEG_BASH}")
endif()

set(dark_ffmpeg_libraries)
foreach(component IN LISTS dark_ffmpeg_components)
  list(APPEND dark_ffmpeg_libraries "${DARK_FFMPEG_PREFIX}/lib/lib${component}.a")
endforeach()

ExternalProject_Add(dark_ffmpeg_external
  SOURCE_DIR "${DARK_FFMPEG_SOURCE}"
  BINARY_DIR "${DARK_FFMPEG_BINARY}"
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND ${dark_ffmpeg_configure_command} ${dark_ffmpeg_configure_args}
  BUILD_COMMAND "${DARK_FFMPEG_MAKE}" -j8
  INSTALL_COMMAND "${DARK_FFMPEG_MAKE}" install
  BUILD_BYPRODUCTS ${dark_ffmpeg_libraries}
  USES_TERMINAL_CONFIGURE TRUE
  USES_TERMINAL_BUILD TRUE
  USES_TERMINAL_INSTALL TRUE
  BUILD_IN_SOURCE FALSE
  UPDATE_COMMAND "")

add_dependencies(dark_ffmpeg dark_ffmpeg_external)
target_link_libraries(dark_ffmpeg INTERFACE ${dark_ffmpeg_libraries})
if(UNIX)
  target_link_libraries(dark_ffmpeg INTERFACE m)
endif()
target_include_directories(dark_ffmpeg SYSTEM INTERFACE
  "${DARK_FFMPEG_PREFIX}/include")
