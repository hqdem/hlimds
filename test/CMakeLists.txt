# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.0)

project(UtopiaTest VERSION 0.0)

set(TEST_TARGET utest)

# Karatsuba multiplier needed for some tests.
set(MULGEN_TARGET mulgen)

add_compile_options(-g3 -Wall -Wignored-qualifiers -DYYERROR_VERBOSE)

# Check if the 'gtest' target has been already imported (e.g., by LLVM).
if(NOT TARGET gtest)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
  )

  # For Windows: Prevent overriding the parent project's compiler/linker settings.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

add_executable(${TEST_TARGET}
  gate/analyzer/prob_estimator_test.cpp
  gate/analyzer/simulation_estimator_test.cpp
  gate/debugger/k_lec_test.cpp
  gate/debugger/lec_test.cpp
  gate/debugger/ril_lec_test.cpp
  gate/debugger/synth_lec_test.cpp
  gate/debugger/verilog_lec_test.cpp
  gate/debugger2/miter2_test.cpp
  gate/model/examples.cpp
  gate/model/gnet_test.cpp
  gate/model2/array_test.cpp
  gate/model2/generator_test.cpp
  gate/model2/list_test.cpp
  gate/model2/net_test.cpp
  gate/model2/seq_net.cpp
  gate/model2/subnet_cnf_encoder_test.cpp
  gate/model2/subnet_test.cpp
  gate/optimizer/associative_balancer_test.cpp
  gate/optimizer/benchmark_optimizer_test.cpp
  gate/optimizer/bgnet_test.cpp
  gate/optimizer/depth_find_test.cpp
  gate/optimizer/find_cone_test.cpp
  gate/optimizer/find_cut_test.cpp
  gate/optimizer/npndb_test.cpp
  gate/optimizer/optimization_count_test.cpp
  gate/optimizer/optimizer_util.cpp
  gate/optimizer/resynthesis/akers_test.cpp
  gate/optimizer/resynthesis/cascade_test.cpp
  gate/optimizer/resynthesis/reed_muller_test.cpp
  gate/optimizer/rwdatabase_test.cpp
  gate/optimizer/substitute_test.cpp
  gate/optimizer/truthtable_test.cpp
  gate/optimizer/util_test.cpp
  gate/optimizer2/cone_builder_test.cpp
  gate/optimizer2/cut_extractor_test.cpp
  gate/optimizer2/resynthesis/akers_test.cpp
  gate/optimizer2/resynthesis/bidecomposition_test.cpp
  gate/optimizer2/resynthesis/cascade_test.cpp
  gate/optimizer2/resynthesis/de_micheli_test.cpp
  gate/optimizer2/resynthesis/isop_test.cpp
  gate/optimizer2/resynthesis/reed_muller_test.cpp
  gate/optimizer2/resynthesis/resynthesis_contest.cpp
  gate/optimizer2/resynthesizer_test.cpp
  gate/parser/graphml_parser_test.cpp
  gate/parser/parser_test.cpp
  gate/premapper/aigmapper/aig_test.cpp
  gate/premapper/mapper/mapper_test.cpp
  gate/premapper/migmapper/migmapper_test.cpp
  gate/premapper/xagmapper/xag_test.cpp
  gate/premapper/xagmapper/xag_verilog_test.cpp
  gate/premapper/xmgmapper/xmgmapper_test.cpp
  gate/printer/dot_test.cpp
  gate/printer/gate_verilog_printer_test.cpp
  gate/printer/graphml_test.cpp
  gate/simulator/simulator_memory_test.cpp
  gate/simulator/simulator_test.cpp
  gate/simulator2/simulator_test.cpp
  gate/tech_mapper/parser_lib_test.cpp
  gate/tech_mapper/techmap_test.cpp
  gate/transformer/aigmapper_test.cpp
  gate/transformer/bdd_test.cpp
  lib/minisat/minisat_test.cpp
  rtl/library/adder_test.cpp
  rtl/library/arithmetic_test.cpp
  rtl/parser/ril/ril_test.cpp
  test_main.cpp
  util/fm_test.cpp
)
target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${TEST_TARGET}
  PRIVATE
    gtest_main
    ${MAIN_LIBRARY}
    easyloggingpp
)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})

file(COPY data DESTINATION .)
