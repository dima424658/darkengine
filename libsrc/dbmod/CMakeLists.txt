add_library(dark_dbmod STATIC
  commands.cpp
  discovery.cpp
  engine_fingerprint.cpp
  fields.cpp
  fingerprint.cpp
  inspection.cpp
  object_reference.cpp
  object_validation.cpp
  property_validation.cpp
  string_fields.cpp
  struct_fields.cpp
  syntax.cpp
)
dark_configure_library(dark_dbmod)
