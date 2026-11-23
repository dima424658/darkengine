add_executable(md_hit_test main.c)
target_link_libraries(md_hit_test PRIVATE dark_md dark_matrix)
add_test(NAME md_hit_detection COMMAND md_hit_test)
