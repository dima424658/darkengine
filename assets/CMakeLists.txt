if(TARGET thief2 AND LINUX AND NOT ANDROID)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/icons/dark-engine-restored.desktop"
    DESTINATION share/applications)
  foreach(icon_size 16 24 32 48 64 128 256 512)
    install(FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/icons/hicolor/${icon_size}x${icon_size}/apps/dark-engine-restored.png"
      DESTINATION "share/icons/hicolor/${icon_size}x${icon_size}/apps")
  endforeach()
endif()
