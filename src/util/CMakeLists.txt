add_library(Util OBJECT
  arith.cpp
  kitty_utils.cpp
  npn_transformation.cpp
  partition_hgraph.cpp
)

target_include_directories(Util
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src)

add_library(Utopia::Util ALIAS Util)

target_link_libraries(Util
  PUBLIC
    Kitty
)
