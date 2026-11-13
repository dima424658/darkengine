add_library(dark_sndsrc STATIC
  fileseg.cpp
  imaadpcm.cpp
  memseg.cpp
  playlist.cpp
  refill.cpp
  rezseg.cpp
  sndseg.cpp
  sndsrc.cpp
  sndsrcid.cpp
)
dark_configure_library(dark_sndsrc)
