add_library(Util OBJECT
  arith.cpp
  fm.cpp
  partition_hgraph.cpp
  string.cpp
)
target_include_directories(Util PUBLIC ${PROJECT_SOURCE_DIR}/src)

add_library(Utopia::Util ALIAS Util)
