# Build the pinned FFmpeg source for the active CMake target. FFmpeg's own
# configure/make build is used because upstream does not provide CMake files.
include(ExternalProject)

set(DARK_FFMPEG_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/FFmpeg")
if(NOT EXISTS "${DARK_FFMPEG_SOURCE}/configure")
  message(FATAL_ERROR
    "FFmpeg source is missing. Initialize submodules with "
    "git submodule update --init --recursive.")
endif()

find_program(DARK_FFMPEG_MAKE NAMES gmake make REQUIRED)
if(WIN32 OR ANDROID)
  find_program(DARK_FFMPEG_BASH NAMES bash REQUIRED)
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
  elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
    set(dark_ffmpeg_arch arm)
  elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
    set(dark_ffmpeg_arch x86_64)
  elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
    set(dark_ffmpeg_arch x86)
  else()
    message(FATAL_ERROR
      "Unsupported Android ABI for FFmpeg: ${CMAKE_ANDROID_ARCH_ABI}")
  endif()
  set(dark_ffmpeg_compiler_target "${CMAKE_C_COMPILER_TARGET}")
  if(NOT dark_ffmpeg_compiler_target)
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
      set(dark_ffmpeg_compiler_target "aarch64-linux-android${CMAKE_SYSTEM_VERSION}")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
      set(dark_ffmpeg_compiler_target "armv7a-linux-androideabi${CMAKE_SYSTEM_VERSION}")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
      set(dark_ffmpeg_compiler_target "x86_64-linux-android${CMAKE_SYSTEM_VERSION}")
    else()
      set(dark_ffmpeg_compiler_target "i686-linux-android${CMAKE_SYSTEM_VERSION}")
    endif()
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
elseif(WIN32)
  list(APPEND dark_ffmpeg_configure_args --toolchain=msvc)
endif()

if(DARK_FFMPEG_BASH)
  set(dark_ffmpeg_configure_command
    "${DARK_FFMPEG_BASH}" "${DARK_FFMPEG_SOURCE}/configure"
    ${dark_ffmpeg_configure_args})
else()
  set(dark_ffmpeg_configure_command
    "${DARK_FFMPEG_SOURCE}/configure" ${dark_ffmpeg_configure_args})
endif()

if(MSVC)
  set(dark_ffmpeg_library_names
    avformat.lib avcodec.lib swresample.lib swscale.lib avutil.lib)
else()
  set(dark_ffmpeg_library_names
    libavformat.a libavcodec.a libswresample.a libswscale.a libavutil.a)
endif()
set(dark_ffmpeg_byproducts)
foreach(library IN LISTS dark_ffmpeg_library_names)
  list(APPEND dark_ffmpeg_byproducts
    "${DARK_FFMPEG_PREFIX}/lib/${library}")
endforeach()

ExternalProject_Add(dark_ffmpeg_external
  SOURCE_DIR "${DARK_FFMPEG_SOURCE}"
  BINARY_DIR "${DARK_FFMPEG_BINARY}"
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND ${dark_ffmpeg_configure_command}
  BUILD_COMMAND "${DARK_FFMPEG_MAKE}" -j8
  INSTALL_COMMAND "${DARK_FFMPEG_MAKE}" install
  BUILD_BYPRODUCTS ${dark_ffmpeg_byproducts}
  USES_TERMINAL_CONFIGURE TRUE
  USES_TERMINAL_BUILD TRUE
  USES_TERMINAL_INSTALL TRUE
  BUILD_IN_SOURCE FALSE
  UPDATE_COMMAND "")

add_library(dark_ffmpeg INTERFACE)
if(MSVC)
  foreach(library IN LISTS dark_ffmpeg_library_names)
    list(APPEND dark_ffmpeg_libraries "${DARK_FFMPEG_PREFIX}/lib/${library}")
  endforeach()
  target_link_libraries(dark_ffmpeg INTERFACE
    ${dark_ffmpeg_libraries} bcrypt secur32 ws2_32 user32 advapi32)
else()
  foreach(library IN LISTS dark_ffmpeg_library_names)
    list(APPEND dark_ffmpeg_libraries
      "${DARK_FFMPEG_PREFIX}/lib/${library}")
  endforeach()
  target_link_libraries(dark_ffmpeg INTERFACE ${dark_ffmpeg_libraries})
  if(UNIX)
    target_link_libraries(dark_ffmpeg INTERFACE m)
  endif()
endif()
target_include_directories(dark_ffmpeg SYSTEM INTERFACE
  "${DARK_FFMPEG_PREFIX}/include")
