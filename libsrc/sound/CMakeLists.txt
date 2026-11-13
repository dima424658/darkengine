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
