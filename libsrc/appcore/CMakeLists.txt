# dappcore.cpp defines a failing legacy factory with the same symbol as the
# SDL factory. Including both lets archive link order select the stub.
add_library(dark_appcore STATIC
  appcore.cpp
  appflags.cpp
  defcreat.cpp
  defexit.cpp
  definit.cpp
  sdl_appcore.cpp
)
dark_configure_library(dark_appcore)
