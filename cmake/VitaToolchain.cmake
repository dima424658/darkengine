if(NOT DEFINED ENV{VITASDK} OR "$ENV{VITASDK}" STREQUAL "")
  message(FATAL_ERROR
    "VitaSDK is not configured. Set VITASDK to the VitaSDK installation directory.")
endif()

set(VITASDK "$ENV{VITASDK}" CACHE PATH "VitaSDK installation directory")
set(VITA_TOOLCHAIN "${VITASDK}/share/vita.toolchain.cmake")

if(NOT EXISTS "${VITA_TOOLCHAIN}")
  message(FATAL_ERROR
    "VitaSDK toolchain not found: ${VITA_TOOLCHAIN}")
endif()

include("${VITA_TOOLCHAIN}")
