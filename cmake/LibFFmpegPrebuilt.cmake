# Consume an architecture-matched Windows FFmpeg SDK without invoking
# FFmpeg's configure/Make build. A custom root may be supplied; otherwise a
# pinned LGPL shared build is downloaded for the active Visual Studio target.
if(NOT WIN32)
  message(FATAL_ERROR "Prebuilt Windows FFmpeg support is Windows-only")
endif()

if(DARK_WINDOWS_FFMPEG_ROOT)
  get_filename_component(DARK_WINDOWS_FFMPEG_ROOT
    "${DARK_WINDOWS_FFMPEG_ROOT}" ABSOLUTE)
else()
  include(FetchContent)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(dark_ffmpeg_archive
      "https://github.com/FFmpeg-Builds/FFmpeg-Builds/releases/download/autobuild-2026-11-21-12-37/ffmpeg-n8.1.3-39-g660b5aa7db-win64-lgpl-shared-8.1.zip")
    set(dark_ffmpeg_hash
      "18E83BAC40326F054226B20FDE94301BA40E22D4F2D752517867F2D8D15CBE9F")
  else()
    set(dark_ffmpeg_archive
      "https://github.com/defisym/FFmpeg-Builds-Win32/releases/download/autobuild-2026-11-20-19-49/ffmpeg-n8.1.3-39-g660b5aa7db-win32-lgpl-shared-8.1.zip")
    set(dark_ffmpeg_hash
      "9F4B8B97EC6252BD3BC7A2A364B74786AA631A63CD42CE24EB89203FD21DBD7D")
  endif()
  FetchContent_Declare(dark_ffmpeg_windows
    URL "${dark_ffmpeg_archive}"
    URL_HASH "SHA256=${dark_ffmpeg_hash}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR dark-no-cmake-project)
  FetchContent_MakeAvailable(dark_ffmpeg_windows)
  set(DARK_WINDOWS_FFMPEG_ROOT "${dark_ffmpeg_windows_SOURCE_DIR}")
  if(NOT EXISTS "${DARK_WINDOWS_FFMPEG_ROOT}/include/libavformat/avformat.h")
    file(GLOB dark_ffmpeg_roots LIST_DIRECTORIES TRUE
      "${DARK_WINDOWS_FFMPEG_ROOT}/ffmpeg-*")
    list(LENGTH dark_ffmpeg_roots dark_ffmpeg_root_count)
    if(dark_ffmpeg_root_count EQUAL 1)
      list(GET dark_ffmpeg_roots 0 DARK_WINDOWS_FFMPEG_ROOT)
    endif()
  endif()
endif()

find_path(DARK_FFMPEG_INCLUDE_DIR libavformat/avformat.h
  PATHS "${DARK_WINDOWS_FFMPEG_ROOT}/include"
  NO_DEFAULT_PATH REQUIRED)

set(dark_ffmpeg_libraries)
foreach(component avformat avcodec swresample swscale avutil)
  string(TOUPPER "${component}" component_upper)
  find_library(DARK_FFMPEG_${component_upper}_LIBRARY
    NAMES "${component}"
    PATHS "${DARK_WINDOWS_FFMPEG_ROOT}/lib"
    NO_DEFAULT_PATH REQUIRED)
  list(APPEND dark_ffmpeg_libraries
    "${DARK_FFMPEG_${component_upper}_LIBRARY}")
endforeach()

add_library(dark_ffmpeg INTERFACE)
target_include_directories(dark_ffmpeg SYSTEM INTERFACE
  "${DARK_FFMPEG_INCLUDE_DIR}")
target_link_libraries(dark_ffmpeg INTERFACE
  ${dark_ffmpeg_libraries} bcrypt secur32 ws2_32 user32 advapi32)
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
  # The upstream Win32 package's GNU import stubs do not carry MSVC SAFESEH
  # metadata. They contain no executable codec implementation; that remains
  # in the DLLs copied beside the application.
  target_link_options(dark_ffmpeg INTERFACE "/SAFESEH:NO")
endif()

file(GLOB DARK_FFMPEG_RUNTIME_DLLS CONFIGURE_DEPENDS
  "${DARK_WINDOWS_FFMPEG_ROOT}/bin/*.dll")
