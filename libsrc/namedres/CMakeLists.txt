add_library(dark_namedres STATIC
  binrstyp.cpp
  bmp.cpp
  cel.cpp
  dds.cpp
  defresm.cpp
  defstfct.cpp
  filestrm.cpp
  fonrstyp.cpp
  gif.cpp
  imgrstyp.cpp
  mdlrstyp.cpp
  palrstyp.cpp
  pathutil.cpp
  pcx.cpp
  png.cpp
  psd.cpp
  resarq.cpp
  resimage.cpp
  resman.cpp
  resmanhs.cpp
  resmanx.cpp
  respall.cpp
  resstat.cpp
  resstr.cpp
  resutil.cpp
  search.cpp
  sndrstyp.cpp
  storbase.cpp
  storedir.cpp
  storeman.cpp
  storezip.cpp
  strrstyp.cpp
  tga.cpp
  zipstfct.cpp
  zipstrm.cpp
)
dark_configure_library(dark_namedres)

target_include_directories(dark_namedres BEFORE PRIVATE "${DARK_PNG_SOURCE}")
target_link_libraries(dark_namedres PUBLIC png_static ZLIB::ZLIBSTATIC BLAST::BLASTSTATIC)
