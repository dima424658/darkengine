cmake_minimum_required(VERSION 3.21)
project(dbmod_test LANGUAGES CXX)
enable_testing()
add_executable(dbmod_test main.cpp ../../libsrc/dbmod/commands.cpp ../../libsrc/dbmod/syntax.cpp ../../libsrc/dbmod/fingerprint.cpp)
target_compile_features(dbmod_test PRIVATE cxx_std_17)
add_test(NAME dbmod_syntax COMMAND dbmod_test)
add_executable(dbmod_fingerprint_test fingerprint.cpp ../../libsrc/dbmod/syntax.cpp
  ../../libsrc/dbmod/fingerprint.cpp)
target_compile_features(dbmod_fingerprint_test PRIVATE cxx_std_17)
add_test(NAME dbmod_fingerprints COMMAND dbmod_fingerprint_test)
add_executable(dbmod_discovery_test discovery.cpp ../../libsrc/dbmod/discovery.cpp)
target_compile_features(dbmod_discovery_test PRIVATE cxx_std_17)
add_test(NAME dbmod_discovery COMMAND dbmod_discovery_test)
add_executable(dbmod_commands_test commands.cpp ../../libsrc/dbmod/commands.cpp
  ../../libsrc/dbmod/syntax.cpp ../../libsrc/dbmod/fingerprint.cpp)
target_compile_features(dbmod_commands_test PRIVATE cxx_std_17)
add_test(NAME dbmod_commands COMMAND dbmod_commands_test)
add_executable(dbmod_fields_test fields.cpp ../../libsrc/dbmod/fields.cpp
  ../../libsrc/dbmod/syntax.cpp)
target_compile_features(dbmod_fields_test PRIVATE cxx_std_17)
add_test(NAME dbmod_fields COMMAND dbmod_fields_test)
add_executable(dbmod_struct_fields_test struct_fields.cpp
  ../../libsrc/dbmod/struct_fields.cpp ../../libsrc/dbmod/fields.cpp
  ../../libsrc/dbmod/syntax.cpp)
target_compile_features(dbmod_struct_fields_test PRIVATE cxx_std_17)
target_include_directories(dbmod_struct_fields_test PRIVATE
  ../../h ../../libsrc/platform ../../libsrc/sdesc)
add_test(NAME dbmod_struct_fields COMMAND dbmod_struct_fields_test)
add_executable(dbmod_string_fields_test string_fields.cpp ../../libsrc/dbmod/string_fields.cpp
  ../../libsrc/dbmod/fields.cpp ../../libsrc/dbmod/syntax.cpp)
target_compile_features(dbmod_string_fields_test PRIVATE cxx_std_17)
add_test(NAME dbmod_string_fields COMMAND dbmod_string_fields_test)

add_executable(dbmod_object_reference_test object_reference.cpp
  ../../libsrc/dbmod/object_reference.cpp)
target_compile_features(dbmod_object_reference_test PRIVATE cxx_std_17)
add_test(NAME dbmod_object_reference COMMAND dbmod_object_reference_test)

get_property(dbmod_test_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(target IN LISTS dbmod_test_targets)
  target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../libsrc/dbmod")
endforeach()
