//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/graphml.h"
#include "graphml_test_utils.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>

using GmlTranslator = eda::gate::translator::GmlTranslator;
using ParserData    = GmlTranslator::ParserData;
using SubnetBuilder = GmlTranslator::Builder;

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

void translate(std::string fileName) {
  ParserData data;
  auto builder = eda::gate::translator::translateGmlOpenabc(fileName, &data);
  EXPECT_TRUE(checkBuilder(builder, data));
}

TEST(GmlTranslator, ac97Ctrl) {
  translate("ac97_ctrl_orig");
}

TEST(GmlTranslator, aes) {
  translate("aes_orig");
}

TEST(GmlTranslator, aesSecworks) {
  translate("aes_secworks_orig");
}

TEST(GmlTranslator, aesXcrypt) {
  translate("aes_xcrypt_orig");
}

TEST(GmlTranslator, apex1) {
  translate("apex1_orig");
}

TEST(GmlTranslator, bc0) {
  translate("bc0_orig");
}

TEST(GmlTranslator, bpBe) {
  translate("bp_be_orig");
}

TEST(GmlTranslator, c1355) {
  translate("c1355_orig");
}

TEST(GmlTranslator, c5315) {
  translate("c5315_orig");
}

TEST(GmlTranslator, c6288) {
  translate("c6288_orig");
}

TEST(GmlTranslator, c7552) {
  translate("c7552_orig");
}

TEST(GmlTranslator, dalu) {
  translate("dalu_orig");
}

TEST(GmlTranslator, des3Area) {
  translate("des3_area_orig");
}

TEST(GmlTranslator, dft) {
  translate("dft_orig");
}

TEST(GmlTranslator, div) {
  translate("div_orig");
}

TEST(GmlTranslator, dynamicNode) {
  translate("dynamic_node_orig");
}

TEST(GmlTranslator, ethernet) {
  translate("ethernet_orig");
}

TEST(GmlTranslator, fir) {
  translate("fir_orig");
}

TEST(GmlTranslator, fpu) {
  translate("fpu_orig");
}

TEST(GmlTranslator, hyp) {
  translate("hyp_orig");
}

TEST(GmlTranslator, i2c) {
  translate("i2c_orig");
}

TEST(GmlTranslator, i10) {
  translate("i10_orig");
}

TEST(GmlTranslator, idft) {
  translate("idft_orig");
}

TEST(GmlTranslator, iir) {
  translate("iir_orig");
}

TEST(GmlTranslator, jpeg) {
  translate("jpeg_orig");
}

TEST(GmlTranslator, k2) {
  translate("k2_orig");
}

TEST(GmlTranslator, log2) {
  translate("log2_orig");
}

TEST(GmlTranslator, mainpla) {
  translate("mainpla_orig");
}

TEST(GmlTranslator, max) {
  translate("max_orig");
}

TEST(GmlTranslator, memCtrl) {
  translate("mem_ctrl_orig");
}

TEST(GmlTranslator, multiplier) {
  translate("multiplier_orig");
}

TEST(GmlTranslator, pci) {
  translate("pci_orig");
}

TEST(GmlTranslator, picosoc) {
  translate("picosoc_orig");
}

TEST(GmlTranslator, sasc) {
  translate("sasc_orig");
}

TEST(GmlTranslator, sha256) {
  translate("sha256_orig");
}

TEST(GmlTranslator, simpleSpi) {
  translate("simple_spi_orig");
}

TEST(GmlTranslator, sin) {
  translate("sin_orig");
}

TEST(GmlTranslator, spi) {
  translate("spi_orig");
}

TEST(GmlTranslator, sqrt) {
  translate("sqrt_orig");
}

TEST(GmlTranslator, square) {
  translate("square_orig");
}

TEST(GmlTranslator, ssPcm) {
  translate("ss_pcm_orig");
}

TEST(GmlTranslator, tinyRocket) {
  translate("tinyRocket_orig");
}

TEST(GmlTranslator, tv80) {
  translate("tv80_orig");
}

TEST(GmlTranslator, usbPhy) {
  translate("usb_phy_orig");
}

TEST(GmlTranslator, vgaLcd) {
  translate("vga_lcd_orig");
}

TEST(GmlTranslator, wbConmax) {
  translate("wb_conmax_orig");
}

TEST(GmlTranslator, wbDma) {
  translate("wb_dma_orig");
}
