//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/graphml.h"
#include "gtest/gtest.h"
#include "gate/printer/dot.h"
#include "util/logging.h"

#include <filesystem>
#include <string>

namespace eda::gate::parser::graphml {

  /**
  * Method checks that numbers of inverted predecessors from graphml file
  * corresponds with those in the built GNet.
  * @return true if no discrepancy is found.
  */
  bool checkAttributes(const GraphMLParser::ParserData &parserData) {
    bool correct = true;
    for (const auto &[id, gate]: parserData.gates) {
      const auto &inputs = model::Gate::get(gate.id)->inputs();
      int realNotsNumber = 0;
      for (const auto &input: inputs) {
        if (model::Gate::get(input.node())->isNot()) {
          ++realNotsNumber;
        }
      }
      if (gate.invertedNumber != realNotsNumber) {
        correct = false;
        LOG_DEBUG("Incorrect NOT's inputs number in gate " << id << '\n'
                      << "Expected: " << gate.invertedNumber << " Found: "
                      << realNotsNumber)
      }
    }
    return correct;
  }

  void parseGraphML(const std::string &infile) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    const std::filesystem::path subCatalog =
        std::filesystem::path("test") / "data" / "gate" / "parser" /
        "graphml" / "OpenABC" / "graphml_openabcd";
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path prefixPathIn = homePath / subCatalog;
    const std::filesystem::path prefixPathOut =
        homePath / subCatalog / "output";
    std::string filename = (prefixPathIn / (infile + ".graphml")).string();

    if (!std::filesystem::exists(filename)) {
      LOG_ERROR << "File " << filename << " doesn't exist!" << std::endl;
      FAIL();
    }

    GraphMLParser::ParserData data;
    auto gNet = GraphMLParser::parse(filename, data);

    EXPECT_TRUE(checkAttributes(data));
    delete gNet;
  }

  TEST(ParserGraphMLTest, ac97Ctrl) {
    parseGraphML("ac97_ctrl_orig.bench");
  }

  TEST(ParserGraphMLTest, aesXcrypt) {
    parseGraphML("aes_xcrypt_orig.bench");
  }

  TEST(ParserGraphMLTest, dft) {
    parseGraphML("dft_orig.bench");
  }

  TEST(ParserGraphMLTest, idft) {
    parseGraphML("idft_orig.bench");
  }

  TEST(ParserGraphMLTest, memCtrl) {
    parseGraphML("mem_ctrl_orig.bench");
  }

  TEST(ParserGraphMLTest, sasc) {
    parseGraphML("sasc_orig.bench");
  }

  TEST(ParserGraphMLTest, spi) {
    parseGraphML("spi_orig.bench");
  }

  TEST(ParserGraphMLTest, tv80) {
    parseGraphML("tv80_orig.bench");
  }

  TEST(ParserGraphMLTest, wbConmax) {
    parseGraphML("wb_conmax_orig.bench");
  }

  TEST(ParserGraphMLTest, aes) {
    parseGraphML("aes_orig.bench");
  }

  TEST(ParserGraphMLTest, bpBe) {
    parseGraphML("bp_be_orig.bench");
  }

  TEST(ParserGraphMLTest, dynamicNode) {
    parseGraphML("dynamic_node_orig.bench");
  }

  TEST(ParserGraphMLTest, fpu) {
    parseGraphML("fpu_orig.bench");
  }

  TEST(ParserGraphMLTest, iir) {
    parseGraphML("iir_orig.bench");
  }

  TEST(ParserGraphMLTest, pci) {
    parseGraphML("pci_orig.bench");
  }

  TEST(ParserGraphMLTest, sha256) {
    parseGraphML("sha256_orig.bench");
  }

  TEST(ParserGraphMLTest, ssPcm) {
    parseGraphML("ss_pcm_orig.bench");
  }

  TEST(ParserGraphMLTest, usbPhy) {
    parseGraphML("usb_phy_orig.bench");
  }

  TEST(ParserGraphMLTest, wbDma) {
    parseGraphML("wb_dma_orig.bench");
  }

  TEST(ParserGraphMLTest, aesSecworks) {
    parseGraphML("aes_secworks_orig.bench");
  }

  TEST(ParserGraphMLTest, des3Area) {
    parseGraphML("des3_area_orig.bench");
  }

  TEST(ParserGraphMLTest, ethernet) {
    parseGraphML("ethernet_orig.bench");
  }

  TEST(ParserGraphMLTest, i2c) {
    parseGraphML("i2c_orig.bench");
  }

  TEST(ParserGraphMLTest, jpeg) {
    parseGraphML("jpeg_orig.bench");
  }

  TEST(ParserGraphMLTest, picosoc) {
    parseGraphML("picosoc_orig.bench");
  }

  TEST(ParserGraphMLTest, simpleSpi) {
    parseGraphML("simple_spi_orig.bench");
  }

  TEST(ParserGraphMLTest, tinyRocket) {
    parseGraphML("tinyRocket_orig.bench");
  }

  TEST(ParserGraphMLTest, vgaLcd) {
    parseGraphML("vga_lcd_orig.bench");
  }
} // namespace eda::gate::parser::graphml