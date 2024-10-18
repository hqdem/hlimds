add_library(Diag OBJECT
  diagnostics.cpp
  processor.cpp
  terminal_printer.cpp
)
add_library(Utopia::Diag ALIAS Diag)


target_compile_options(Diag
  PRIVATE
    -fno-rtti
)

target_include_directories(Diag
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(Diag
  PUBLIC
    easyloggingpp
    fmt
)
