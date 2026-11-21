cmake_minimum_required(VERSION 3.21)
project(dataops_test LANGUAGES CXX)
enable_testing()
add_executable(dataops_test main.cpp ../../src/object/dataops.cpp)
target_compile_features(dataops_test PRIVATE cxx_std_17)
target_compile_definitions(dataops_test PRIVATE NDEBUG STRICT=1)
target_compile_options(dataops_test PRIVATE -include types.h)
target_include_directories(dataops_test PRIVATE
  ../../h
  ../../inc
  ../../src/object
  ../../src/engfeat
  ../../src/script
  ../../src/dark
  ../../src/render
  ../../libsrc/matrix
  ../../libsrc/platform
  ../../libsrc/platform/windows
  ../../libsrc/comtools
  ../../libsrc/compapis
  ../../libsrc/lg
  ../../libsrc/lgalloc
  ../../libsrc/tagfile
  ../../libsrc/cpptools)
add_test(NAME dataops COMMAND dataops_test)
