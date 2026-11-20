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
  material.cpp
  material_parser.cpp
  material_lexer.cpp
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

dark_generate_parser(material_parser material.y
  material_parser/material_grammar.cpp material_parser/material_grammar.hpp
  COMPILE_FLAGS "--warnings=error")
target_sources(dark_namedres PRIVATE ${material_parser_sources})
target_include_directories(dark_namedres PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/material_parser")
