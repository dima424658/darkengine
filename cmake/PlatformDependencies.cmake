# Shared SDL3 and desktop media/image package discovery.
find_package(SDL3 CONFIG REQUIRED)
set(DARK_SDL_TARGET SDL3::SDL3)

if(ANDROID)
  find_library(DARK_ANDROID_LOG_LIBRARY NAMES log REQUIRED)
else()
  find_package(PNG REQUIRED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat libavcodec libavutil libswscale libswresample)
endif()
