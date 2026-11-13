target_sources(thief2 PRIVATE main.cpp)
if(TARGET dromed)
  target_sources(dromed PRIVATE main.cpp)
endif()
