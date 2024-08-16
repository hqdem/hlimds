add_compile_options(-fno-rtti)
add_library(Diag OBJECT
  diagnostics.cpp
  processor.cpp
  terminal_printer.cpp
)
add_library(Utopia::Diag ALIAS Diag)

target_include_directories(Diag
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/lib
)

target_link_libraries(Diag
  PRIVATE
    easyloggingpp
    fmt
)
