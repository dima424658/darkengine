add_library(dark_dump STATIC
  bmpdump.c
  filefind.c
  gifdump.c
  pcxdump.c
)
dark_configure_library(dark_dump)
