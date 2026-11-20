add_executable(win32_compat_test main.cpp c_interface.c)
target_link_libraries(win32_compat_test PRIVATE dark_platform)
add_test(NAME win32_compat COMMAND win32_compat_test)
