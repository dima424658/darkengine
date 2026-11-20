add_library(dark_cpptools STATIC
  delmlist.cpp
  dlist.cpp
  dynarray.cpp
  dynarsrv.cpp
  dynfunc.cpp
  filesystem.cpp
  growblk.cpp
  hashpp.cpp
  hashset.cpp
  packflag.cpp
  pdynarr.cpp
  relocptr.cpp
  splitpat.cpp
  str.cpp
)
dark_configure_library(dark_cpptools)

target_sources(dark_platform PRIVATE dynfunc.cpp)
