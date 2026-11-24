set(_package_root "${BINARY_DIR}/vita-test-vpk")
file(REMOVE_RECURSE "${_package_root}")

set(_packages
  "THFG00001|Thief: The Dark Project|ux0:data/thief1"
  "THFG00002|Thief Gold|ux0:data/thiefgold"
  "THFG00003|Thief II: The Metal Age|ux0:data/thief2"
)

foreach(_entry IN LISTS _packages)
  string(REPLACE "|" ";" _fields "${_entry}")
  list(GET _fields 0 _title_id)
  list(GET _fields 1 _title)
  list(GET _fields 2 _asset_path)

  set(_package_dir "${_package_root}/${_title_id}")
  file(MAKE_DIRECTORY "${_package_dir}/sce_sys")
  file(MAKE_DIRECTORY "${_package_dir}/data")
  file(MAKE_DIRECTORY "${_package_dir}/assets")

  file(WRITE "${_package_dir}/sce_sys/package.json"
    "{\n"
    "  \"title_id\": \"${_title_id}\",\n"
    "  \"title\": \"${_title}\",\n"
    "  \"asset_path\": \"${_asset_path}\",\n"
    "  \"render_resolution\": \"960x544\",\n"
    "  \"platform\": \"PS Vita\"\n"
    "}\n")

  file(WRITE "${_package_dir}/assets/README.txt"
    "Place retail assets under ${_asset_path}\n")
endforeach()

file(WRITE "${_package_root}/README.txt"
  "Thief Trilogy Vita test VPK structure\n"
  "Three LiveArea package roots are staged: THFG00001, THFG00002, THFG00003.\n")

message(STATUS "Vita test package structure created at ${_package_root}")

if(VITASDK AND EXISTS "${VITASDK}/bin/vita-pack-vpk")
  message(STATUS "vita-pack-vpk detected; full EBOOT packaging is deferred until TASK 1 engine bring-up.")
else()
  message(STATUS "VitaSDK packaging tools unavailable; directory layout generated as a placeholder.")
endif()
