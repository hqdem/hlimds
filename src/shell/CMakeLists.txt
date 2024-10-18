add_library(Shell OBJECT
  shell.cpp
)
add_library(Utopia::Shell ALIAS Shell)

target_include_directories(Shell
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src

  PRIVATE
    ${PROJECT_BINARY_DIR}/src # config.h
)

target_link_libraries(Shell
  PUBLIC
    CLI
    fmt
    Tcl::Tcl
    Utopia::Diag
    Utopia::Gate
)
