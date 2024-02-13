# FindSTACCATO.cmake

# Define a cmake module to find STACCATO library
# This script sets the following variables:
# - STACCATO_FOUND: true if STACCATO library is found
# - STACCATO_INCLUDE_DIRS: include directories for STACCATO
# - STACCATO_LIBRARIES: libraries to link against for STACCATO

# Set the search path for libraries and headers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

find_library(STACCATO_LIBRARY
  NAMES libSTACCATO.so
  HINTS "/usr/local/lib"
)

find_path(STACCATO_INCLUDE_DIRS
  NAMES
  DSDDecompose.h
  DSD.h
  DSDInterface.h
  DSDManager.h
  DSDManipulations.h
  DSDNewDecompose.h
  DSDOrDecompose.h
  DSDPrimeDecompose.h
  DSDUtilities.h
  DSDXorDecompose.h
  fixheap.h
  HINTS "/usr/local/include"
)

# Check if STACCATO library and headers are found
if(STACCATO_LIBRARY AND STACCATO_INCLUDE_DIRS)
  set(STACCATO_FOUND TRUE)
else()
  set(STACCATO_FOUND FALSE)
endif()

# Provide feedback to the user
if(STACCATO_FOUND)
  message(STATUS "STACCATO library found")
  message(STATUS "STACCATO_INCLUDE_DIRS: ${STACCATO_INCLUDE_DIRS}")
  message(STATUS "STACCATO_LIBRARY: ${STACCATO_LIBRARY}")
else()
  message(STATUS "STACCATO library not found")
endif()

# Export the variables for external use
if(STACCATO_FOUND)
  set(STACCATO_LIBRARIES ${STACCATO_LIBRARY})
  mark_as_advanced(STACCATO_LIBRARY STACCATO_INCLUDE_DIRS)
endif()
