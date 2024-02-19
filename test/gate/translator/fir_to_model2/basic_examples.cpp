//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/fir_to_model2/fir_to_model2.h"
#include "gate/model2/printer/printer.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace eda::gate::model {

enum InputFormat { InputMLIRFile, InputFIRFile };

int firrtlTranslatorTest(const std::string &inputFileName,
    const std::string &outputFileName,
    const InputFormat inputFormat = InputFormat::InputMLIRFile) {
  // Parse the input 'FIRRTL' file.
  const fs::path homePath = std::string(getenv("UTOPIA_HOME"));
  static constexpr const char *relativeInputPath =
      "test/data/gate/fir_to_model2";
  static constexpr const char *relativeOutputPath =
      "output/test/gate/fir_to_model2";
  const fs::path inputFullPath = homePath / relativeInputPath;
  const fs::path inputFullName = inputFullPath / inputFileName;
  Translator translator{inputFormat == InputFormat::InputFIRFile ?
      MLIRModule::loadFromFIRFile(inputFullName.c_str()) :
      MLIRModule::loadFromMLIRFile(inputFullName.c_str())};

  // Print the input 'FIRRTL' code. 
  translator.printFIRRTL();

  // Convert the 'FIRRTL' representation to the 'model2' representation.
  const auto resultNetlist = translator.translate();

  // Dump the output net to the console.
  for (const auto &cellTypeID : *resultNetlist) {
    std::cout << CellType::get(cellTypeID).getNet() << std::endl;
  }

  // Dump the output net to the '.dot' file.
  const fs::path outputFullPath = homePath /
                                  relativeOutputPath;
  fs::create_directories(outputFullPath);
  const fs::path outputFullName = outputFullPath / outputFileName;
  std::ofstream outputStream(outputFullName);
  for (const auto &cellTypeID : *resultNetlist) {
    outputStream << CellType::get(cellTypeID).getNet() << std::endl;
  }
  outputStream.close();

  return 0;
}

// 'MLIR' tests.
TEST(FIRRTLTranslatorTestBasic, InToOutTest) {
  EXPECT_EQ(firrtlTranslatorTest("in_to_out.mlir", "in_to_out.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, OutToTest) {
  EXPECT_EQ(firrtlTranslatorTest("out_to.mlir", "out_to.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleMuxTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_mux.mlir", "simple_mux.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleAddTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_add.mlir", "simple_add.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, TwoLevelAddTest) {
  EXPECT_EQ(firrtlTranslatorTest("two_level_add.mlir", "two_level_add.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleInstanceTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_instance.mlir",
                                 "simple_instance.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, TwoLevelInstanceTest) {
  EXPECT_EQ(firrtlTranslatorTest("two_level_instance.mlir",
                                 "two_level_instance.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleXorTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_xor.mlir", "simple_xor.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, TwoLevelXorTest) {
  EXPECT_EQ(firrtlTranslatorTest("two_level_xor.mlir", "two_level_xor.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleRegisterTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_reg.mlir", "simple_reg.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleRegisterWithResetTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_regreset.mlir",
                                 "simple_regreset.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleConstantTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_constant.mlir",
                                 "simple_constant.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, DotProductTest) {
  EXPECT_EQ(firrtlTranslatorTest("dot_product.mlir", "dot_product.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, AddSameInputsTest) {
  EXPECT_EQ(firrtlTranslatorTest("add_same_inputs.mlir",
                                 "add_same_inputs.dot"), 0);
}

TEST(FIRRTLTranslatorTestBasic, AddInstanceMixTest) {
  EXPECT_EQ(firrtlTranslatorTest("add_instance_mix.mlir",
                                 "add_instance_mix.dot"), 0);
}

// 'Picorv' tests.
TEST(FIRRTLTranslatorTestPicorv, Fir0Test) {
  EXPECT_EQ(firrtlTranslatorTest("fir_0.fir",
                                 "fir_0.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestPicorv, Fir1Test) {
  EXPECT_EQ(firrtlTranslatorTest("fir_1.fir",
                                 "fir_1.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestPicorv, Fir2Test) {
  EXPECT_EQ(firrtlTranslatorTest("fir_2.fir",
                                 "fir_2.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestPicorv, Fir3Test) {
  EXPECT_EQ(firrtlTranslatorTest("fir_3.fir",
                                 "fir_3.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestPicorv, CompareFirTest) {
  EXPECT_EQ(firrtlTranslatorTest("compare.fir",
                                 "compare.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestPicorv, CatFirTest) {
  EXPECT_EQ(firrtlTranslatorTest("cat.fir",
                                 "cat.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestPicorv, CondFirTest) {
  EXPECT_EQ(firrtlTranslatorTest("cond.fir",
                                 "cond.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestPicorv, CycleFirTest) {
  EXPECT_EQ(firrtlTranslatorTest("cycle.fir",
                                 "cycle.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestPicorv, SinFirTest) {
  EXPECT_EQ(firrtlTranslatorTest("sin.fir",
                                 "sin.dot",
                                 InputFIRFile), 0);
}

// 'FIRRTL 3.2.0' specification tests.
TEST(FIRRTLTranslatorTestSpec, SpecCircuitsTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_circuits.fir",
                                 "spec_circuits.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecGroupsTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_groups.fir",
                                 "spec_groups.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecNestedGroupsTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_nested_groups.fir",
                                 "spec_nested_groups.dot",
                                 InputFIRFile), 0);
}

/// TODO: Not supported (for now).
// TEST(FIRRTLTranslatorTestSpec, SpecGroupsDefineTest) {
//   EXPECT_EQ(firrtlTranslatorTest("spec_groups_define.fir",
//                                  "spec_groups_define.dot",
//                                  InputFIRFile), 0);
// }

TEST(FIRRTLTranslatorTestSpec, SpecExternalModulesTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_extmodules.fir",
                                 "spec_extmodules.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecExternalModulesRefTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_extmodules_ref.fir",
                                 "spec_extmodules_ref.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecIntrinsicModulesRefTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_intmodules.fir",
                                 "spec_intmodules.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecProbesTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_probes.fir",
                                 "spec_probes.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecProbesInferTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_probes_infer.fir",
                                 "spec_probes_infer.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecAliasesTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_aliases.fir",
                                 "spec_aliases.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecSkipTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_skip.fir",
                                 "spec_skip.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecInvalidateTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_invalidate.fir",
                                 "spec_invalidate.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecWhenTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_when.fir",
                                 "spec_when.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecWhenShortTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_when_short.fir",
                                 "spec_when_short.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecMultipleWhenTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_multiple_when.fir",
                                 "spec_multiple_when.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecWhenOneLineTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_when_one_line.fir",
                                 "spec_when_one_line.dot",
                                 InputFIRFile), 0);
}

/// TODO: Ask about the strange lowered circuit (tag, subtag, zero width, etc.). 
// TEST(FIRRTLTranslatorTestSpec, SpecMatchTest) {
//   EXPECT_EQ(firrtlTranslatorTest("spec_match.fir",
//                                  "spec_match.dot",
//                                  InputFIRFile), 0);
// }

TEST(FIRRTLTranslatorTestSpec, SpecNestedDeclarationsTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_nested_decls.fir",
                                 "spec_nested_decls.dot",
                                 InputFIRFile), 0);
}

/// TODO: Ask why the lowered circuit contains bundles. 
// TEST(FIRRTLTranslatorTestSpec, SpecMemoryTest) {
//   EXPECT_EQ(firrtlTranslatorTest("spec_mem.fir",
//                                  "spec_mem.dot",
//                                  InputFIRFile), 0);
// }

TEST(FIRRTLTranslatorTestSpec, SpecInstTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_inst.fir",
                                 "spec_inst.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecStopTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_stop.fir",
                                 "spec_stop.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecPrintfTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_printf.fir",
                                 "spec_printf.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecAssumeTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_assume.fir",
                                 "spec_assume.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecCoverTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_cover.fir",
                                 "spec_cover.dot",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecPropertyAssignmentTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_propassign.fir",
                                 "spec_propassign.dot",
                                 InputFIRFile), 0);
}

} // namespace eda::gate::model
