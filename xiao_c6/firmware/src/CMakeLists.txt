# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# Make headers under ../include available as normal includes, e.g.
#   #include "config.h"
#   #include "protocol/hazard_packet.h"
#   #include "haptics/haptics.h"
idf_component_register(
  SRCS ${app_sources}
  INCLUDE_DIRS "." "../include"
)
