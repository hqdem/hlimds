add_library(Gate OBJECT
  debugger/checker.cpp
  debugger/encoder.cpp
  debugger/symexec.cpp
  model/gate.cpp
  model/gnet.cpp
  model/gsymbol.cpp
  optimizer/database/abc/rwrUtil.c
  optimizer/database/abc.cpp
  optimizer/check_cut.cpp
  optimizer/cone_visitor.cpp
  optimizer/cuts_finder_visitor.cpp
  optimizer/links_clean.cpp
  optimizer/optimizer.cpp
  optimizer/optimizer_visitor.cpp
  optimizer/rwdatabase.cpp
  optimizer/rwmanager.cpp
  optimizer/tracker_visitor.cpp
  optimizer/tracker_visitor.h
  optimizer/util.cpp
  optimizer/walker.cpp
  parser/gate_verilog_parser.cpp
  premapper/aigmapper.cpp
<<<<<<< HEAD
  premapper/xagmapper.cpp
  premapper/premapper.cpp
#  printer/hmetis_printer.cpp
=======
  premapper/migmapper.cpp
  premapper/premapper.cpp
  premapper/xmgmapper.cpp
>>>>>>> 523867b7910603532e01602fb47a696b39be2379
  printer/dot.cpp
  simulator/simulator.cpp 
  simulator/simulator.cpp
  transformer/bdd.cpp
  transformer/hmetis.cpp)
add_library(Utopia::Gate ALIAS Gate)

target_include_directories(Gate PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(Gate
  PUBLIC
    minisat-lib-static
    Cudd::Cudd
    sqlite3

  PRIVATE
    Utopia::Util
)
