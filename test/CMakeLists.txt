# SPDX-License-Identifier: Apache-2.0

# Set policy to operate with timestamps of extracted files
cmake_policy(SET CMP0135 NEW)

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
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )

  # For Windows: Prevent overriding the parent project's compiler/linker settings.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

add_executable(${TEST_TARGET}
  gate/analyzer/prob_estimator_test.cpp
  gate/analyzer/simulation_estimator_test.cpp
  gate/debugger/merge_test.cpp
  gate/debugger/miter_test.cpp
  gate/debugger/register_sweep_test.cpp
  gate/debugger/sat_checker_test.cpp
  gate/debugger/seq_sweep_test.cpp
  gate/debugger/synth_lec_test.cpp
  gate/debugger/verifier_test.cpp
  gate/estimator/time_model_test.cpp
  gate/estimator/wlm_test.cpp
  gate/model/array_test.cpp
  gate/model/design_analysis_test.cpp
  gate/model/design_test.cpp
  gate/model/examples.cpp
  gate/model/generator_test.cpp
  gate/model/list_test.cpp
  gate/model/net_decomposer_test.cpp
  gate/model/net_test.cpp
  gate/model/replace_test.cpp
  gate/model/seq_net.cpp
  gate/model/serializer_test.cpp
  gate/model/subnet_depth_test.cpp
  gate/model/subnet_test.cpp
  gate/model/utils/bdd_dnf_test.cpp
  gate/model/utils/subnet_truth_table_test.cpp
  gate/mutator/mutator_test.cpp
  gate/premapper/aigmapper_test.cpp
  gate/optimizer/associative_balancer_test.cpp
  gate/optimizer/cone_builder_test.cpp
  gate/optimizer/cut_extractor_test.cpp
  gate/optimizer/getdbstat_test.cpp
  gate/optimizer/npndb_test.cpp
  gate/optimizer/npndbserial_test.cpp
  gate/optimizer/npndbstat_test.cpp
  gate/optimizer/pass_test.cpp
  gate/optimizer/reconvergence_cut_test.cpp
  gate/optimizer/refactor_test.cpp
  gate/optimizer/resubstitutor_test.cpp
  gate/optimizer/resynthesizer_test.cpp
  gate/optimizer/rewriter_test.cpp
  gate/optimizer/rwdatabase_test.cpp
  gate/optimizer/synthesis/abc_npn4_test.cpp
  gate/optimizer/synthesis/akers_test.cpp
  gate/optimizer/synthesis/algebraic_factor_test.cpp
  gate/optimizer/synthesis/bidecomposition_test.cpp
  gate/optimizer/synthesis/cascade_test.cpp
  gate/optimizer/synthesis/de_micheli_test.cpp
  gate/optimizer/synthesis/dsd_test.cpp
  gate/optimizer/synthesis/isop_test.cpp
  gate/optimizer/synthesis/reed_muller_test.cpp
  gate/optimizer/synthesis/zhegalkin_test.cpp
  gate/premapper/aigmapper_test.cpp
  gate/simulator/simulator_test.cpp
  gate/subnet_to_bdd/subnet_to_bdd_test.cpp
  gate/synthesizer/adder_test.cpp
  gate/synthesizer/cmp_test.cpp
  gate/synthesizer/shift_test.cpp
  gate/techmapper/parser_lib_test.cpp
  gate/techmapper/strategy/area_recovery_test.cpp
  gate/techmapper/strategy/net_mapping_test.cpp
  gate/techmapper/strategy/power_test.cpp
  gate/techmapper/strategy/simple_area_test.cpp
  gate/techmapper/strategy/simple_delay_test.cpp
  gate/techmapper/techmapper_test_util.cpp
  gate/translator/fir/fir_model2_test.cpp
  gate/translator/graphml_test_utils.cpp
  gate/translator/graphml_test.cpp
  lib/minisat/minisat_test.cpp
  rtl/parser/ril/ril_test.cpp
  test_main.cpp
  util/kitty_utils_test.cpp
  util/serializer_test.cpp
)
target_include_directories(${TEST_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${TEST_TARGET}
  PRIVATE
    gtest_main
    ${MAIN_LIBRARY}
    ${ELPP_LIBRARY}
)

include(GoogleTest)
gtest_discover_tests(${TEST_TARGET})

file(COPY data DESTINATION .)
