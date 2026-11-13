add_library(dark_lgalloc STATIC
  allcguid.cpp
  heap.cpp
  memall.c
  membase.cpp
  memcore.cpp
  multpool.cpp
  nonover.cpp
  nullallc.cpp
  pool.cpp
  poolimp.cpp
  primallc.cpp
  stdalloc.cpp
)
dark_configure_library(dark_lgalloc)
