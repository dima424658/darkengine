add_library(dark_sound STATIC
  factory.cpp
  fade.cpp
  lgsndid.cpp
  mixer.cpp
  openal_loader.cpp
  openal.cpp
  platform.cpp
  sample.cpp
  sndfmt.cpp
  timelog.cpp
  volconv.cpp
)
dark_configure_library(dark_sound)

target_compile_definitions(dark_sound PRIVATE
  $<$<PLATFORM_ID:Windows>:DARK_OPENAL_LIBRARY="OpenAL32.dll">
  $<$<PLATFORM_ID:Android>:DARK_OPENAL_LIBRARY="libopenal.so">
  $<$<PLATFORM_ID:Darwin>:DARK_OPENAL_LIBRARY="/System/Library/Frameworks/OpenAL.framework/OpenAL">
  $<$<PLATFORM_ID:Linux>:DARK_OPENAL_LIBRARY="libopenal.so.1">
)
