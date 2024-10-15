//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/npn.h"
#include "gate/optimizer/npndb.h"
#include "gate/optimizer/synthesis/db_mig4_synthesizer.h"
#include "gate/optimizer/synthesis/db_xag4_synthesizer.h"
#include "gate/translator/logdb.h"

#include "gtest/gtest.h"

namespace eda::gate::translator {

using DbSynthesizer = optimizer::synthesis::DbSynthesizer;

static std::string toHexString(uint8_t k, uint64_t value) {
  assert(k <= 6);
  assert(value <= (1ull << (1 << k)) - 1);

  std::stringstream ss;
  const uint8_t shift = k < 2 ? 0 : k - 2;
  ss << std::setfill('0') << std::setw(1 << shift) << std::hex << value;
  return ss.str();
}

static void checkNpn4Database(const DbSynthesizer &dbSyn) {
  for (size_t i = 0; i < optimizer::npn4Num; ++i) {
    kitty::dynamic_truth_table tt(4);
    kitty::dynamic_truth_table care(4);
    kitty::create_from_hex_string(tt, toHexString(4, optimizer::npn4[i]));

    const auto object = dbSyn.synthesize(tt, care, 2);
    const auto &subnet = gate::model::Subnet::get(object.id());

    const auto subnetTT = gate::model::evaluateSingleOut(subnet);
    EXPECT_EQ(tt, subnetTT);
  }
}

TEST(LogdbTranslator, xag4) {
  const auto &xag4Synthesizer = optimizer::synthesis::DbXag4Synthesizer::get();
  checkNpn4Database(xag4Synthesizer);
}

TEST(LogdbTranslator, mig4) {
  const auto &mig4Synthesizer = optimizer::synthesis::DbMig4Synthesizer::get();
  checkNpn4Database(mig4Synthesizer);
}

} // namespace eda::gate::translator
