add_library(Shell OBJECT
  shell.cpp
)
add_library(Utopia::Shell ALIAS Shell)

target_include_directories(Shell
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR}/.. # config.h
)

target_link_libraries(Shell
  PRIVATE
    CLI
    easyloggingpp
    fmt
    Kitty
    readcells
    Tcl::Tcl
    Yosys::Yosys
)
