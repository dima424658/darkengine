if(TARGET dark_ffmpeg)
  add_library(dark_actmovie STATIC
    movie.cpp
    movie_audio_output.cpp
  )
else()
  add_library(dark_actmovie STATIC movie_windows.cpp)
endif()
dark_configure_library(dark_actmovie)

if(TARGET dark_ffmpeg)
  target_link_libraries(dark_actmovie PUBLIC dark_ffmpeg)
else()
  target_link_libraries(dark_actmovie PRIVATE mfplat mf mfuuid mfplay)
endif()
