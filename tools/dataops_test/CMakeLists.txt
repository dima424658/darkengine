cmake_minimum_required(VERSION 3.21)
project(dataops_test LANGUAGES CXX)
enable_testing()
add_executable(dataops_test main.cpp ../../src/object/dataops.cpp)
set_target_properties(dataops_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_compile_definitions(dataops_test PRIVATE NDEBUG STRICT=1)
target_compile_options(dataops_test PRIVATE -include types.h)
target_include_directories(dataops_test PRIVATE
  ../../h
  ../../inc
  ../../src/object
  ../../src/engfeat
  ../../src/ai
  ../../src/portal
  ../../src/sim
  ../../src/sound
  ../../src/framewrk
  ../../libsrc/dstruct
  ../../libsrc/mprintf
  ../../src/script
  ../../src/dark
  ../../src/render
  ../../libsrc/matrix
  ../../libsrc/gfile
  ../../libsrc/platform
  ../../libsrc/platform/windows
  ../../libsrc/comtools
  ../../libsrc/compapis
  ../../libsrc/lg
  ../../libsrc/lgalloc
  ../../libsrc/tagfile
  ../../libsrc/cpptools)
add_test(NAME dataops COMMAND dataops_test)
