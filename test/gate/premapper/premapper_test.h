//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/sat_checker.h"
#include "gate/model/celltype.h"
#include "gate/model/subnet.h"
#include "gate/model/utils/subnet_random.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/premapper/premapper.h"

#include "gtest/gtest.h"

#include <iostream>
#include <vector>

namespace eda::gate::premapper {

using Builder    = eda::gate::model::SubnetBuilder;
using CellSymbol = eda::gate::model::CellSymbol;
using Link       = eda::gate::model::Subnet::Link;
using LinkList   = eda::gate::model::Subnet::LinkList;
using SatChecker = eda::gate::debugger::SatChecker;
using Subnet     = eda::gate::model::Subnet;
using SubnetID   = eda::gate::model::SubnetID;

static SubnetID createPrimitiveSubnet(CellSymbol symbol, size_t nIn, size_t arity) {
  Builder builder;
  LinkList links = builder.addInputs(nIn);

  const auto link = builder.addCellTree(symbol, links, arity);
  builder.addOutput(link);

  return builder.make();
}

static void checkSimpleCases(const Premapper *premapper) {
  SatChecker &checker = SatChecker::get();
  Builder builder1;
  builder1.addInput();
  const auto xor1 = builder1.addCell(CellSymbol::XOR, Link(0, 0), Link(0, 0));
  builder1.addOutput(xor1);
  const auto id1 = builder1.make();
  const auto transformed1 = premapper->transform(id1);
  EXPECT_TRUE(checker.areEquivalent(id1, transformed1).equal());

  Builder builder2;
  builder2.addInput();
  Link or2;
  or2 = builder2.addCell(CellSymbol::OR, Link(0, 0), Link(0, 1), Link(0, 0));
  builder2.addOutput(or2);
  const auto id2 = builder2.make();
  const auto transformed2 = premapper->transform(id2);
  EXPECT_TRUE(checker.areEquivalent(id2, transformed2).equal());
}

static void checkConstCases(const Premapper *premapper) {
  SatChecker &checker = SatChecker::get();
  Builder builder1;
  builder1.addInput();

  const Link one1 = builder1.addCell(CellSymbol::ONE);
  const Link zero1 = builder1.addCell(CellSymbol::ZERO);

  LinkList links1{Link(0, 0), Link(0, 0), Link(0, 1), zero1, one1};
  const auto or1 = builder1.addCell(CellSymbol::OR, links1);
  builder1.addOutput(or1);
  const auto id1 = builder1.make();
  const auto transformed1 = premapper->transform(id1);
  EXPECT_TRUE(checker.areEquivalent(id1, transformed1).equal());

  Builder builder2;
  builder2.addInput();

  const Link one2 = builder2.addCell(CellSymbol::ONE);
  const Link zero2 = builder2.addCell(CellSymbol::ZERO);

  LinkList links2{one2, Link(0, 0), Link(0, 1), one2, zero2, one2, zero2};
  const auto xor2 = builder2.addCell(CellSymbol::XOR, links2);
  builder2.addOutput(xor2);
  const auto id2 = builder2.make();
  const auto transformed2 = premapper->transform(id2);
  EXPECT_TRUE(checker.areEquivalent(id2, transformed2).equal());
}

static void checkMAJ(const Premapper *premapper) {
  const size_t nIn = 5;
  Builder builder;
  LinkList links;

  for (size_t i = 0; i < nIn; ++i) {
    const auto link = builder.addInput();
    links.push_back(i % 2 ? ~link : link);
  }

  links.push_back(builder.addCell(CellSymbol::MAJ, links));
  builder.addOutput(links.back());

  const auto id = builder.make();
  const auto transformed = premapper->transform(id);

  EXPECT_EQ(eda::gate::model::evaluate(Subnet::get(id)),
            eda::gate::model::evaluate(Subnet::get(transformed)));
}

static void checkAND(const Premapper *premapper) {
  SatChecker &checker = SatChecker::get();
  const auto andSub  = createPrimitiveSubnet(CellSymbol::AND, 13, 7);
  const auto andTree = createPrimitiveSubnet(CellSymbol::AND, 13, 2);

  const auto transformedAndSub  = premapper->transform(andSub);
  const auto transformedAndTree = premapper->transform(andTree);

  EXPECT_TRUE(checker.areEquivalent(andSub, transformedAndSub).equal());
  EXPECT_TRUE(checker.areEquivalent(andTree, transformedAndTree).equal());
}

static void checkOR(const Premapper *premapper) {
  SatChecker &checker = SatChecker::get();
  const auto orSub  = createPrimitiveSubnet(CellSymbol::OR, 13, 7);
  const auto orTree = createPrimitiveSubnet(CellSymbol::OR, 13, 2);

  const auto transformedOrSub  = premapper->transform(orSub);
  const auto transformedOrTree = premapper->transform(orTree);

  EXPECT_TRUE(checker.areEquivalent(orSub, transformedOrSub).equal());
  EXPECT_TRUE(checker.areEquivalent(orTree, transformedOrTree).equal());
}

static void checkXOR(const Premapper *premapper) {
  SatChecker &checker = SatChecker::get();
  const auto xorSub  = createPrimitiveSubnet(CellSymbol::XOR, 13, 7);
  const auto xorTree = createPrimitiveSubnet(CellSymbol::XOR, 13, 2);

  const auto transformedXorSub  = premapper->transform(xorSub);
  const auto transformedXorTree = premapper->transform(xorTree);

  EXPECT_TRUE(checker.areEquivalent(xorSub, transformedXorSub).equal());
  EXPECT_TRUE(checker.areEquivalent(xorTree, transformedXorTree).equal());
}

static void checkRandomSubnet(const Premapper *premapper) {
  const size_t nIn      = 10u;
  const size_t nOut     = 1u;
  const size_t nCell    = 60u;
  const size_t MinArity = 1u;
  const size_t MaxArity = 6u;
  const size_t nLoops   = 20u;

  for (size_t i = 0; i < nLoops; ++i) {
    const auto id = eda::gate::model::randomSubnet(nIn, nOut, nCell,
                                                   MinArity, MaxArity);

    const auto &oldSubnet = Subnet::get(id);
    const auto transformed = premapper->transform(id);
    const auto &transformedSubnet = Subnet::get(transformed);

    EXPECT_EQ(eda::gate::model::evaluate(oldSubnet),
              eda::gate::model::evaluate(transformedSubnet));
  }
}

} // namespace eda::gate::premapper
