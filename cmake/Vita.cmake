if(DARK_TARGET_VITA)
  if(NOT DARK_VITA_PLACEHOLDER)
    if(NOT DEFINED ENV{VITASDK} OR "$ENV{VITASDK}" STREQUAL "")
      message(FATAL_ERROR
        "DARK_TARGET_VITA requires VITASDK. Install VitaSDK and set VITASDK "
        "to its installation directory.")
    endif()
  endif()

  set(DARK_BUILD_DROMED OFF CACHE BOOL "Vita port does not build DromEd" FORCE)
  set(DARK_COMPILE_VULKAN_SHADERS OFF CACHE BOOL "Vita uses vitaGL" FORCE)

  set(VITA_PACKAGES
    THFG00001_THIEF1
    THFG00002_THIEFGOLD
    THFG00003_THIEF2
  )

  if(DARK_VITA_PLACEHOLDER)
    add_executable(vita_path_test
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/vita_path_test.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/platform/vita_paths.cpp")
    target_include_directories(vita_path_test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/libsrc/platform")
  endif()

  add_custom_target(vita-test-vpk
    COMMAND "${CMAKE_COMMAND}"
      "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DBINARY_DIR=${CMAKE_BINARY_DIR}"
      "-DVITASDK=$ENV{VITASDK}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VitaPackageTest.cmake"
    COMMENT "Creating the Vita test package structure"
    VERBATIM)

  if(TARGET vita_path_test)
    add_dependencies(vita-test-vpk vita_path_test)
    add_custom_command(TARGET vita-test-vpk POST_BUILD
      COMMAND $<TARGET_FILE:vita_path_test>
      COMMENT "Running Vita asset path routing test"
      VERBATIM)
  endif()
endif()
