add_library(dark_script STATIC
  multparm.cpp
  nullsrv.cpp
  osm_emu/osm32abi.cpp
  osm_emu/osm32bridge.cpp
  osm_emu/osm32cpu.cpp
  osm_emu/osm32env.cpp
  osm_emu/osm32img.cpp
  osm_emu/osm32imports.cpp
  osm_emu/osm32mod.cpp
  osm_emu/osm32rt.cpp
  persist.cpp
  scrobj.cpp
  scrptman.cpp
  squirrel/squirrelhost.cpp
  scrptmsg.cpp
  smodinfo.cpp
)
dark_configure_library(dark_script)

if(DARK_ENABLE_SQUIRREL)
  target_compile_definitions(dark_script PRIVATE DARK_ENABLE_SQUIRREL=1)
  target_link_libraries(dark_script PRIVATE dark_squirrel)
endif()
