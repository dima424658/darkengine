if(NOT DARK_TARGET_VITA)
  return()
endif()

if(NOT DEFINED ENV{VITASDK} OR "$ENV{VITASDK}" STREQUAL "")
  message(FATAL_ERROR
    "DARK_TARGET_VITA requires VITASDK to point to a VitaSDK installation.")
endif()

include("$ENV{VITASDK}/share/vita.cmake")

find_package(SDL2 REQUIRED)
find_path(DARK_VITAGL_INCLUDE_DIR vitaGL.h REQUIRED)
find_library(DARK_VITAGL_LIBRARY NAMES vitaGL REQUIRED)

if(NOT TARGET SDL2::SDL2)
  message(FATAL_ERROR
    "The Vita SDL2 package did not define the expected SDL2::SDL2 target.")
endif()

set(DARK_VITA_PLATFORMS
  "thief1-vita|THFG00001|Thief: The Dark Project|ux0:data/thief1"
  "thiefgold-vita|THFG00002|Thief Gold|ux0:data/thiefgold"
  "thief2-vita|THFG00003|Thief II: The Metal Age|ux0:data/thief2"
)

set(_vita_vpk_targets)
foreach(_platform IN LISTS DARK_VITA_PLATFORMS)
  string(REPLACE "|" ";" _fields "${_platform}")
  list(GET _fields 0 _target)
  list(GET _fields 1 _title_id)
  list(GET _fields 2 _title)
  list(GET _fields 3 _asset_root)

  add_executable("${_target}" "${PROJECT_SOURCE_DIR}/tools/vita_bootstrap.c")
  target_include_directories("${_target}" PRIVATE "${DARK_VITAGL_INCLUDE_DIR}")
  target_compile_features("${_target}" PRIVATE c_std_11)
  target_compile_definitions("${_target}" PRIVATE
    "DARK_VITA_TITLE=\"${_title}\""
    "DARK_VITA_TITLE_ID=\"${_title_id}\""
    "DARK_VITA_ASSET_ROOT=\"${_asset_root}\""
  )
  target_link_libraries("${_target}" PRIVATE
    SDL2::SDL2
    "${DARK_VITAGL_LIBRARY}"
    SceAppUtil_stub
    SceCtrl_stub
    SceDisplay_stub
    SceGxm_stub
    SceKernel_stub
    SceSysmodule_stub
    SceTouch_stub
    m
  )

  vita_create_self("${_target}.self" "${_target}")
  vita_create_vpk("${_target}.vpk" "${_title_id}" "${_target}.self"
    VERSION "01.00"
    NAME "${_title}"
  )
  add_dependencies("${_target}.vpk-vpk" "${_target}.self-self")
  list(APPEND _vita_vpk_targets "${_target}.vpk-vpk")
endforeach()

add_custom_target(vita-trilogy-vpk DEPENDS ${_vita_vpk_targets})

message(STATUS "Configured Vita ARM target: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "VPK targets: ${_vita_vpk_targets}")
