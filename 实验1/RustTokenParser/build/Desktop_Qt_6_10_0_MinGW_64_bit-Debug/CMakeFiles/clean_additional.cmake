# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\RustTokenParser_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\RustTokenParser_autogen.dir\\ParseCache.txt"
  "RustTokenParser_autogen"
  )
endif()
