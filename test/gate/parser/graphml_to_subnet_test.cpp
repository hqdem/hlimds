//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/graphml_to_subnet.h"
#include "util/assert.h"

#include <filesystem>
#include <string>

#include "gtest/gtest.h"

using GraphMlSubnetParser = eda::gate::parser::graphml::GraphMlSubnetParser;
using ParserData          = GraphMlSubnetParser::ParserData;
using Subnet              = GraphMlSubnetParser::Subnet;

bool checkSubnet(const Subnet &subnet, const ParserData &data) {
  bool res{true};
  for (const auto &node : data.nodes) {
    size_t invIns{0};
    auto links = subnet.getLinks(node.id);
    for (const auto &link : links) {
      invIns += link.inv;
    }
    res &= (node.invIns == invIns);
  }
  return res;
}

void parseGraphML(std::string fileName) {
  uassert(getenv("UTOPIA_HOME"), "UTOPIA_HOME is not set" << std::endl);

  using path = std::filesystem::path;

  fileName += ".bench.graphml";

  const path dir = path("test") / "data" / "gate" / "parser"
      / "graphml" / "OpenABC" / "graphml_openabcd";
  const path home = std::string(getenv("UTOPIA_HOME"));
  const path file = home / dir / fileName;

  uassert(std::filesystem::exists(file.string()),
                                  "File doesn't exist" << std::endl);

  GraphMlSubnetParser parser;
  ParserData data;
  const auto &subnet = Subnet::get(parser.parse(file.string(), data));
  //std::cout << subnet << std::endl;
  EXPECT_TRUE(checkSubnet(subnet, data));
}

// TEST(GraphMlSubnetParserTest, ac97Ctrl) {
//   parseGraphML("ac97_ctrl_orig");
// }

// TEST(GraphMlSubnetParserTest, aesXcrypt) {
//   parseGraphML("aes_xcrypt_orig");
// }

// TEST(GraphMlSubnetParserTest, dft) {
//   parseGraphML("dft_orig");
// }

// TEST(GraphMlSubnetParserTest, idft) {
//   parseGraphML("idft_orig");
// }

// TEST(GraphMlSubnetParserTest, memCtrl) {
//   parseGraphML("mem_ctrl_orig");
// }

// TEST(GraphMlSubnetParserTest, sasc) {
//   parseGraphML("sasc_orig");
// }

// TEST(GraphMlSubnetParserTest, spi) {
//   parseGraphML("spi_orig");
// }

// TEST(GraphMlSubnetParserTest, tv80) {
//   parseGraphML("tv80_orig");
// }

// TEST(GraphMlSubnetParserTest, wbConmax) {
//   parseGraphML("wb_conmax_orig");
// }

// TEST(GraphMlSubnetParserTest, aes) {
//   parseGraphML("aes_orig");
// }

// TEST(GraphMlSubnetParserTest, bpBe) {
//   parseGraphML("bp_be_orig");
// }

// TEST(GraphMlSubnetParserTest, dynamicNode) {
//   parseGraphML("dynamic_node_orig");
// }

// TEST(GraphMlSubnetParserTest, fpu) {
//   parseGraphML("fpu_orig");
// }

// TEST(GraphMlSubnetParserTest, iir) {
//   parseGraphML("iir_orig");
// }

// TEST(GraphMlSubnetParserTest, pci) {
//   parseGraphML("pci_orig");
// }

// TEST(GraphMlSubnetParserTest, sha256) {
//   parseGraphML("sha256_orig");
// }

// TEST(GraphMlSubnetParserTest, ssPcm) {
//   parseGraphML("ss_pcm_orig");
// }

// TEST(GraphMlSubnetParserTest, usbPhy) {
//   parseGraphML("usb_phy_orig");
// }

// TEST(GraphMlSubnetParserTest, wbDma) {
//   parseGraphML("wb_dma_orig");
// }

// TEST(GraphMlSubnetParserTest, aesSecworks) {
//   parseGraphML("aes_secworks_orig");
// }

// TEST(GraphMlSubnetParserTest, des3Area) {
//   parseGraphML("des3_area_orig");
// }

// TEST(GraphMlSubnetParserTest, ethernet) {
//   parseGraphML("ethernet_orig");
// }

// TEST(GraphMlSubnetParserTest, i2c) {
//   parseGraphML("i2c_orig");
// }

// TEST(GraphMlSubnetParserTest, jpeg) {
//   parseGraphML("jpeg_orig");
// }

// TEST(GraphMlSubnetParserTest, picosoc) {
//   parseGraphML("picosoc_orig");
// }

// TEST(GraphMlSubnetParserTest, simpleSpi) {
//   parseGraphML("simple_spi_orig");
// }

// TEST(GraphMlSubnetParserTest, tinyRocket) {
//   parseGraphML("tinyRocket_orig");
// }

// TEST(GraphMlSubnetParserTest, vgaLcd) {
//   parseGraphML("vga_lcd_orig");
// }
