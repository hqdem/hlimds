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

using Format = eda::gate::model::ModelPrinter::Format;

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
#ifdef UTOPIA_DEBUG
  translator.printFIRRTL();
#endif
  // Convert the 'FIRRTL' representation to the 'model2' representation.
  const auto resultNetlist = translator.translate();

  // Dump the output net to the console (Format::SIMPLE).
#ifdef UTOPIA_DEBUG
  for (const auto &cellTypeID : *resultNetlist) {
    std::cout << CellType::get(cellTypeID).getNet() << std::endl;
  }
#endif
  // Dump the output net to the '.v' file.
  const fs::path outputFullPath = homePath / relativeOutputPath;
  fs::create_directories(outputFullPath);
  const fs::path outputFullName = outputFullPath / outputFileName;
  std::ofstream outputStream(outputFullName);
  for (const auto &cellTypeID : *resultNetlist) {
      ModelPrinter::getPrinter(Format::VERILOG).print(outputStream,
          CellType::get(cellTypeID).getNet());
  }
  outputStream.close();

  return 0;
}

// 'MLIR' tests.
TEST(FIRRTLTranslatorTestBasic, InToOutTest) {
  EXPECT_EQ(firrtlTranslatorTest("in_to_out.mlir", "in_to_out.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, OutToTest) {
  EXPECT_EQ(firrtlTranslatorTest("out_to.mlir", "out_to.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleMuxTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_mux.mlir", "simple_mux.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleAddTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_add.mlir", "simple_add.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, TwoLevelAddTest) {
  EXPECT_EQ(firrtlTranslatorTest("two_level_add.mlir", "two_level_add.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleInstanceTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_instance.mlir",
                                 "simple_instance.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, TwoLevelInstanceTest) {
  EXPECT_EQ(firrtlTranslatorTest("two_level_instance.mlir",
                                 "two_level_instance.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleXorTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_xor.mlir", "simple_xor.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, TwoLevelXorTest) {
  EXPECT_EQ(firrtlTranslatorTest("two_level_xor.mlir", "two_level_xor.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleRegisterTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_reg.mlir", "simple_reg.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleRegisterWithResetTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_regreset.mlir",
                                 "simple_regreset.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, SimpleConstantTest) {
  EXPECT_EQ(firrtlTranslatorTest("simple_constant.mlir",
                                 "simple_constant.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, DotProductTest) {
  EXPECT_EQ(firrtlTranslatorTest("dot_product.mlir", "dot_product.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, AddSameInputsTest) {
  EXPECT_EQ(firrtlTranslatorTest("add_same_inputs.mlir",
                                 "add_same_inputs.v"), 0);
}

TEST(FIRRTLTranslatorTestBasic, AddInstanceMixTest) {
  EXPECT_EQ(firrtlTranslatorTest("add_instance_mix.mlir",
                                 "add_instance_mix.v"), 0);
}

// 'FIRRTL 3.2.0' specification tests.
TEST(FIRRTLTranslatorTestSpec, SpecCircuitsTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_circuits.fir",
                                 "spec_circuits.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecGroupsTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_groups.fir",
                                 "spec_groups.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecNestedGroupsTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_nested_groups.fir",
                                 "spec_nested_groups.v",
                                 InputFIRFile), 0);
}

/// TODO: Not supported (for now).
// TEST(FIRRTLTranslatorTestSpec, SpecGroupsDefineTest) {
//   EXPECT_EQ(firrtlTranslatorTest("spec_groups_define.fir",
//                                  "spec_groups_define.v",
//                                  InputFIRFile), 0);
// }

TEST(FIRRTLTranslatorTestSpec, SpecExternalModulesTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_extmodules.fir",
                                 "spec_extmodules.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecExternalModulesRefTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_extmodules_ref.fir",
                                 "spec_extmodules_ref.v",
                                 InputFIRFile), 0);
}

/// TODO: Not supported (for now).
// TEST(FIRRTLTranslatorTestSpec, SpecIntrinsicModulesRefTest) {
//   EXPECT_EQ(firrtlTranslatorTest("spec_intmodules.fir",
//                                  "spec_intmodules.v",
//                                  InputFIRFile), 0);
// }

TEST(FIRRTLTranslatorTestSpec, SpecProbesTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_probes.fir",
                                 "spec_probes.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecProbesInferTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_probes_infer.fir",
                                 "spec_probes_infer.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecAliasesTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_aliases.fir",
                                 "spec_aliases.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecSkipTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_skip.fir",
                                 "spec_skip.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecInvalidateTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_invalidate.fir",
                                 "spec_invalidate.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecWhenTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_when.fir",
                                 "spec_when.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecWhenShortTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_when_short.fir",
                                 "spec_when_short.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecMultipleWhenTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_multiple_when.fir",
                                 "spec_multiple_when.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecWhenOneLineTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_when_one_line.fir",
                                 "spec_when_one_line.v",
                                 InputFIRFile), 0);
}

/// TODO: Ask about the strange lowered circuit (tag, subtag, zero width, etc.). 
// TEST(FIRRTLTranslatorTestSpec, SpecMatchTest) {
//   EXPECT_EQ(firrtlTranslatorTest("spec_match.fir",
//                                  "spec_match.v",
//                                  InputFIRFile), 0);
// }

TEST(FIRRTLTranslatorTestSpec, SpecNestedDeclarationsTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_nested_decls.fir",
                                 "spec_nested_decls.v",
                                 InputFIRFile), 0);
}

/// TODO: Ask why the lowered circuit contains bundles. 
// TEST(FIRRTLTranslatorTestSpec, SpecMemoryTest) {
//   EXPECT_EQ(firrtlTranslatorTest("spec_mem.fir",
//                                  "spec_mem.v",
//                                  InputFIRFile), 0);
// }

TEST(FIRRTLTranslatorTestSpec, SpecInstTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_inst.fir",
                                 "spec_inst.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecStopTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_stop.fir",
                                 "spec_stop.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecPrintfTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_printf.fir",
                                 "spec_printf.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecAssumeTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_assume.fir",
                                 "spec_assume.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecCoverTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_cover.fir",
                                 "spec_cover.v",
                                 InputFIRFile), 0);
}

TEST(FIRRTLTranslatorTestSpec, SpecPropertyAssignmentTest) {
  EXPECT_EQ(firrtlTranslatorTest("spec_propassign.fir",
                                 "spec_propassign.v",
                                 InputFIRFile), 0);
}

} // namespace eda::gate::model
