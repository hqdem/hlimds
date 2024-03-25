add_library(Util OBJECT
  arith.cpp
  fm.cpp
  kitty_utils.cpp
  npn_transformation.cpp
  partition_hgraph.cpp
  string.cpp
)

target_include_directories(Util
  PUBLIC
    ${PROJECT_SOURCE_DIR}/lib/kitty/include
    ${PROJECT_SOURCE_DIR}/src)

add_library(Utopia::Util ALIAS Util)
