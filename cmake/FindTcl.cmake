find_package(TCL QUIET)

if(TCL_FOUND)
  set(Tcl_Tcl_FOUND TRUE)
endif()

if(TK_FOUND)
  set(Tcl_Tk_FOUND TRUE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Tcl
  REQUIRED_VARS
    TCL_LIBRARY
    TCL_INCLUDE_PATH

  HANDLE_COMPONENTS
)

if(Tcl_FOUND)
  if(NOT TARGET Tcl::Tcl)
    add_library(Tcl::Tcl UNKNOWN IMPORTED)
    set_target_properties(Tcl::Tcl PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${TCL_INCLUDE_PATH}"
      IMPORTED_LOCATION "${TCL_LIBRARY}"
    )
  endif()

  if(NOT TARGET Tcl::Tk AND TK_FOUND)
    add_library(Tcl::Tk UNKNOWN IMPORTED)
    set_target_properties(Tcl::Tk PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${TK_INCLUDE_PATH}"
      IMPORTED_LOCATION "${TK_LIBRARY}"
    )
  endif()
endif()
