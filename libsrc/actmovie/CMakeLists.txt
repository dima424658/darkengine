add_library(dark_actmovie STATIC
  movie.cpp
  movie_audio_output.cpp
)
dark_configure_library(dark_actmovie)

target_link_libraries(dark_actmovie PUBLIC dark_ffmpeg)
