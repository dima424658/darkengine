add_executable(material_test main.cpp)
target_link_libraries(material_test PRIVATE dark_namedres)
add_test(NAME materials COMMAND material_test)
