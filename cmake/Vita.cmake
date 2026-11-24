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

  add_custom_target(vita-test-vpk
    COMMAND "${CMAKE_COMMAND}"
      "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
      "-DBINARY_DIR=${CMAKE_BINARY_DIR}"
      "-DVITASDK=$ENV{VITASDK}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VitaPackageTest.cmake"
    COMMENT "Creating the Vita test package structure"
    VERBATIM)
endif()
