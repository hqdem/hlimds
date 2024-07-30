//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/graphml.h"
#include "graphml_test_utils.h"

#include "gtest/gtest.h"

#include <filesystem>
#include <string>

namespace eda::gate::translator::graphml {

using GmlTranslator = eda::gate::translator::GmlTranslator;
using ParserData    = GmlTranslator::ParserData;
using SubnetBuilder = GmlTranslator::Builder;
using Subnet = eda::gate::model::Subnet; 
void checkBuilder(const std::shared_ptr<SubnetBuilder> &builder,
                  const ParserData &data) {
  for (const auto &[id, node] : data.nodes) {
    auto links = builder->getLinks(node.link.value().idx);
    EXPECT_EQ(node.inputs.size(), links.size());
    size_t invIns{0};
    for (size_t i{0}; i < links.size(); ++i) {
      invIns += links[i].inv;
      EXPECT_EQ(links[i].idx, node.inputs[i].node->link.value().idx);
    }
    EXPECT_EQ(node.invIns, invIns);
  }
}

void translate(std::string fileName) {
  ParserData data;
  auto builder = eda::gate::translator::translateGmlOpenabc(fileName, &data);
  checkBuilder(builder, data);
}

TEST(GmlTranslator, ac97Ctrl) {
  translate("ac97_ctrl_orig");
}

TEST(GmlTranslator, c1355) {
  translate("c1355_orig");
}

TEST(GmlTranslator, c5315) {
  translate("c5315_orig");
}

TEST(GmlTranslator, c7552) {
  translate("c7552_orig");
}

TEST(GmlTranslator, i2c) {
  translate("i2c_orig");
}

TEST(GmlTranslator, sasc) {
  translate("sasc_orig");
}

TEST(GmlTranslator, simpleSpi) {
  translate("simple_spi_orig");
}

TEST(GmlTranslator, ssPcm) {
  translate("ss_pcm_orig");
}

TEST(GmlTranslator, usbPhy) {
  translate("usb_phy_orig");
}

TEST(GmlTranslator, wbConmax) {
  translate("wb_conmax_orig");
}

} // namespace eda::gate::translator::graphml
