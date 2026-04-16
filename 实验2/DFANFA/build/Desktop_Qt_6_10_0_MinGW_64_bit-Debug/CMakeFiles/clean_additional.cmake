# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\DFANFA_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\DFANFA_autogen.dir\\ParseCache.txt"
  "DFANFA_autogen"
  )
endif()
