add_library(dark_res STATIC
  lzw.cpp
  lzwfd2bf.cpp
  pkzip.cpp
  refacc.cpp
  res.cpp
  resacc.cpp
  resagg.c
  resbuild.cpp
  rescache.cpp
  rescback.cpp
  rescum.cpp
  resexblk.cpp
  resfile.cpp
  resguid.c
  resload.cpp
  resmake.cpp
  resmem.cpp
  resmisc.cpp
  resthred.cpp
  restypes.cpp
)
dark_configure_library(dark_res)

target_link_libraries(dark_res PUBLIC BLAST::BLASTSTATIC)
