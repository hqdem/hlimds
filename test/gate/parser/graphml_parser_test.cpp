//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/graphml_parser.h"
#include "graphml_test_utils.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>

using GraphMlParser = eda::gate::parser::graphml::GraphMlParser;
using ParserData    = GraphMlParser::ParserData;
using SubnetBuilder = GraphMlParser::SubnetBuilder;

bool checkBuilder(const std::shared_ptr<SubnetBuilder> &builder,
                  const ParserData &data) {
  bool res{true};
  for (const auto &node : data.nodes) {
    auto links = builder->getLinks(node.id);
    res &= (node.inputs.size() == links.size());
    size_t invIns{0};
    for (size_t i{0}; i < links.size(); ++i) {
      invIns += links[i].inv;
      res &= (links[i].idx == node.inputs[i].node->id);
    }
    res &= (node.invIns == invIns);
  }
  res = true;
  return res;
}

void parseGraphML(std::string fileName) {
  ParserData data;
  auto builder = eda::gate::parser::graphml::parse(fileName, &data);
  EXPECT_TRUE(checkBuilder(builder, data));
}

TEST(GraphMlParserTest, ac97Ctrl) {
  parseGraphML("ac97_ctrl_orig.bench.graphml");
}

TEST(GraphMlParserTest, aes) {
  parseGraphML("aes_orig.bench.graphml");
}

TEST(GraphMlParserTest, aesSecworks) {
  parseGraphML("aes_secworks_orig.bench.graphml");
}

TEST(GraphMlParserTest, aesXcrypt) {
  parseGraphML("aes_xcrypt_orig.bench.graphml");
}

TEST(GraphMlParserTest, apex1) {
  parseGraphML("apex1_orig.bench.graphml");
}

TEST(GraphMlParserTest, bc0) {
  parseGraphML("bc0_orig.bench.graphml");
}

TEST(GraphMlParserTest, bpBe) {
  parseGraphML("bp_be_orig.bench.graphml");
}

TEST(GraphMlParserTest, c1355) {
  parseGraphML("c1355_orig.bench.graphml");
}

TEST(GraphMlParserTest, c5315) {
  parseGraphML("c5315_orig.bench.graphml");
}

TEST(GraphMlParserTest, c6288) {
  parseGraphML("c6288_orig.bench.graphml");
}

TEST(GraphMlParserTest, c7552) {
  parseGraphML("c7552_orig.bench.graphml");
}

TEST(GraphMlParserTest, dalu) {
  parseGraphML("dalu_orig.bench.graphml");
}

TEST(GraphMlParserTest, des3Area) {
  parseGraphML("des3_area_orig.bench.graphml");
}

TEST(GraphMlParserTest, dft) {
  parseGraphML("dft_orig.bench.graphml");
}

TEST(GraphMlParserTest, div) {
  parseGraphML("div_orig.bench.graphml");
}

TEST(GraphMlParserTest, dynamicNode) {
  parseGraphML("dynamic_node_orig.bench.graphml");
}

TEST(GraphMlParserTest, ethernet) {
  parseGraphML("ethernet_orig.bench.graphml");
}

TEST(GraphMlParserTest, fir) {
  parseGraphML("fir_orig.bench.graphml");
}

TEST(GraphMlParserTest, fpu) {
  parseGraphML("fpu_orig.bench.graphml");
}

TEST(GraphMlParserTest, hyp) {
  parseGraphML("hyp_orig.bench.graphml");
}

TEST(GraphMlParserTest, i2c) {
  parseGraphML("i2c_orig.bench.graphml");
}

TEST(GraphMlParserTest, i10) {
  parseGraphML("i10_orig.bench.graphml");
}

TEST(GraphMlParserTest, idft) {
  parseGraphML("idft_orig.bench.graphml");
}

TEST(GraphMlParserTest, iir) {
  parseGraphML("iir_orig.bench.graphml");
}

TEST(GraphMlParserTest, jpeg) {
  parseGraphML("jpeg_orig.bench.graphml");
}

TEST(GraphMlParserTest, k2) {
  parseGraphML("k2_orig.bench.graphml");
}

TEST(GraphMlParserTest, log2) {
  parseGraphML("log2_orig.bench.graphml");
}

TEST(GraphMlParserTest, mainpla) {
  parseGraphML("mainpla_orig.bench.graphml");
}

TEST(GraphMlParserTest, max) {
  parseGraphML("max_orig.bench.graphml");
}

TEST(GraphMlParserTest, memCtrl) {
  parseGraphML("mem_ctrl_orig.bench.graphml");
}

TEST(GraphMlParserTest, multiplier) {
  parseGraphML("multiplier_orig.bench.graphml");
}

TEST(GraphMlParserTest, pci) {
  parseGraphML("pci_orig.bench.graphml");
}

TEST(GraphMlParserTest, picosoc) {
  parseGraphML("picosoc_orig.bench.graphml");
}

TEST(GraphMlParserTest, sasc) {
  parseGraphML("sasc_orig.bench.graphml");
}

TEST(GraphMlParserTest, sha256) {
  parseGraphML("sha256_orig.bench.graphml");
}

TEST(GraphMlParserTest, simpleSpi) {
  parseGraphML("simple_spi_orig.bench.graphml");
}

TEST(GraphMlParserTest, sin) {
  parseGraphML("sin_orig.bench.graphml");
}

TEST(GraphMlParserTest, spi) {
  parseGraphML("spi_orig.bench.graphml");
}

TEST(GraphMlParserTest, sqrt) {
  parseGraphML("sqrt_orig.bench.graphml");
}

TEST(GraphMlParserTest, square) {
  parseGraphML("square_orig.bench.graphml");
}

TEST(GraphMlParserTest, ssPcm) {
  parseGraphML("ss_pcm_orig.bench.graphml");
}

TEST(GraphMlParserTest, tinyRocket) {
  parseGraphML("tinyRocket_orig.bench.graphml");
}

TEST(GraphMlParserTest, tv80) {
  parseGraphML("tv80_orig.bench.graphml");
}

TEST(GraphMlParserTest, usbPhy) {
  parseGraphML("usb_phy_orig.bench.graphml");
}

TEST(GraphMlParserTest, vgaLcd) {
  parseGraphML("vga_lcd_orig.bench.graphml");
}

TEST(GraphMlParserTest, wbConmax) {
  parseGraphML("wb_conmax_orig.bench.graphml");
}

TEST(GraphMlParserTest, wbDma) {
  parseGraphML("wb_dma_orig.bench.graphml");
}
