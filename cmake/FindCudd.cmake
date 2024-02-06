find_path(Cudd_INCLUDE_DIR "cudd.h" PATH_SUFFIXES include)

if (NOT Cudd_LIBRARY)
  set(ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  find_library(Cudd_LIBRARY_STATIC cudd PATH_SUFFIXES lib)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".so")
  find_library(Cudd_LIBRARY_SHARED cudd PATH_SUFFIXES lib)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

  if (Cudd_LIBRARY_STATIC)
    set(Cudd_LIBRARY ${Cudd_LIBRARY_STATIC})
  else()
    set(Cudd_LIBRARY ${Cudd_LIBRARY_SHARED})
  endif()
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Cudd
  REQUIRED_VARS
    Cudd_LIBRARY
    Cudd_INCLUDE_DIR
)

if(Cudd_FOUND)
  set(Cudd_INCLUDE_DIRS ${Cudd_INCLUDE_DIR})

  if(NOT Cudd_LIBRARIES)
    set(Cudd_LIBRARIES ${Cudd_LIBRARY})
  endif()

  if (NOT TARGET Cudd::Cudd)
    add_library(Cudd::Cudd UNKNOWN IMPORTED)
    set_target_properties(Cudd::Cudd PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Cudd_INCLUDE_DIRS}"
      IMPORTED_LOCATION "${Cudd_LIBRARY}")
  endif()
  if (NOT TARGET Cudd::static AND Cudd_LIBRARY_STATIC)
    add_library(Cudd::static UNKNOWN IMPORTED)
    set_target_properties(Cudd::static PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Cudd_INCLUDE_DIRS}"
      IMPORTED_LOCATION "${Cudd_LIBRARY_STATIC}")
  endif()
  if (NOT TARGET Cudd::shared AND Cudd_LIBRARY_SHARED)
    add_library(Cudd::shared UNKNOWN IMPORTED)
    set_target_properties(Cudd::shared PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Cudd_INCLUDE_DIRS}"
      IMPORTED_LOCATION "${Cudd_LIBRARY_SHARED}")
  endif()
endif()
