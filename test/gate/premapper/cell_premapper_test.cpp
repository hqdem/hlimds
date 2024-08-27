//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/model/celltype.h"
#include "gate/model/subnet.h"
#include "gate/model/utils/subnet_random.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/premapper/premapper.h"

#include "gtest/gtest.h"

#include <memory>

namespace eda::gate::premapper {

using Builder      = eda::gate::model::SubnetBuilder;
using BuilderPtr   = std::shared_ptr<Builder>;
using CellSymbol   = eda::gate::model::CellSymbol;
using Link         = eda::gate::model::Subnet::Link;
using LinkList     = eda::gate::model::Subnet::LinkList;
using SatChecker   = eda::gate::debugger::SatChecker;
using Subnet       = eda::gate::model::Subnet;
using SubnetID     = eda::gate::model::SubnetID;
using SubnetMapper = eda::gate::optimizer::SubnetMapper;

static BuilderPtr createPrimitiveSubnet(CellSymbol symbol,
                                        uint16_t nIn,
                                        uint16_t arity) {

  BuilderPtr builder = std::make_shared<Builder>();
  LinkList links = builder->addInputs(nIn);

  const auto link = builder->addCellTree(symbol, links, arity);
  builder->addOutput(link);

  return builder;
}

static void checkSimpleCases(const SubnetMapper &premapper) {
  SatChecker &checker = SatChecker::get();
  BuilderPtr builder1 = std::make_shared<Builder>();
  builder1->addInput();
  const auto xor1 = builder1->addCell(CellSymbol::XOR, Link(0, 0), Link(0, 0));
  builder1->addOutput(xor1);

  const auto mapped1 = premapper->map(builder1);
  EXPECT_TRUE(checker.areEquivalent(builder1->make(), mapped1->make()).equal());

  BuilderPtr builder2 = std::make_shared<Builder>();
  builder2->addInput();
  Link or2;
  or2 = builder2->addCell(CellSymbol::OR, Link(0, 0), Link(0, 1), Link(0, 0));
  builder2->addOutput(or2);
  const auto mapped2 = premapper->map(builder2);
  EXPECT_TRUE(checker.areEquivalent(builder2->make(), mapped2->make()).equal());
}

static void checkConstCases(const SubnetMapper &premapper) {
  SatChecker &checker = SatChecker::get();
  BuilderPtr builder1 = std::make_shared<Builder>();
  builder1->addInput();

  const Link one1 = builder1->addCell(CellSymbol::ONE);
  const Link zero1 = builder1->addCell(CellSymbol::ZERO);

  LinkList links1{Link(0, 0), Link(0, 0), Link(0, 1), zero1, one1};
  const auto or1 = builder1->addCell(CellSymbol::OR, links1);
  builder1->addOutput(or1);
  const auto mapped1 = premapper->map(builder1);
  EXPECT_TRUE(checker.areEquivalent(builder1->make(), mapped1->make()).equal());

  BuilderPtr builder2 = std::make_shared<Builder>();
  builder2->addInput();

  const Link one2 = builder2->addCell(CellSymbol::ONE);
  const Link zero2 = builder2->addCell(CellSymbol::ZERO);

  LinkList links2{one2, Link(0, 0), Link(0, 1), one2, zero2, one2, zero2};
  const auto xor2 = builder2->addCell(CellSymbol::XOR, links2);
  builder2->addOutput(xor2);
  const auto mapped2 = premapper->map(builder2);
  EXPECT_TRUE(checker.areEquivalent(builder2->make(), mapped2->make()).equal());
}

static void checkMAJ(const SubnetMapper &premapper) {
  const uint16_t nIn = 5;
  BuilderPtr builder = std::make_shared<Builder>();
  LinkList links;

  for (uint16_t i = 0; i < nIn; ++i) {
    const auto link = builder->addInput();
    links.push_back(i % 2 ? ~link : link);
  }

  links.push_back(builder->addCell(CellSymbol::MAJ, links));
  builder->addOutput(links.back());

  const auto mapped = premapper->map(builder);

  EXPECT_EQ(eda::gate::model::evaluate(Subnet::get(builder->make())),
            eda::gate::model::evaluate(Subnet::get(mapped->make())));
}

static void checkAND(const SubnetMapper &premapper) {
  SatChecker &checker = SatChecker::get();
  const auto and1 = createPrimitiveSubnet(CellSymbol::AND, 13, 7);
  const auto and2 = createPrimitiveSubnet(CellSymbol::AND, 13, 2);

  const auto mappedAnd1 = premapper->map(and1);
  const auto mappedAnd2 = premapper->map(and2);

  EXPECT_TRUE(checker.areEquivalent(and1->make(), mappedAnd1->make()).equal());
  EXPECT_TRUE(checker.areEquivalent(and2->make(), mappedAnd2->make()).equal());
}

static void checkOR(const SubnetMapper &premapper) {
  SatChecker &checker = SatChecker::get();
  const auto or1 = createPrimitiveSubnet(CellSymbol::OR, 13, 7);
  const auto or2 = createPrimitiveSubnet(CellSymbol::OR, 13, 2);

  const auto mappedOr1 = premapper->map(or1);
  const auto mappedOr2 = premapper->map(or2);

  EXPECT_TRUE(checker.areEquivalent(or1->make(), mappedOr1->make()).equal());
  EXPECT_TRUE(checker.areEquivalent(or2->make(), mappedOr2->make()).equal());
}

static void checkXOR(const SubnetMapper &premapper) {
  SatChecker &checker = SatChecker::get();
  const auto xor1 = createPrimitiveSubnet(CellSymbol::XOR, 13, 7);
  const auto xor2 = createPrimitiveSubnet(CellSymbol::XOR, 13, 2);

  const auto mappedXor1 = premapper->map(xor1);
  const auto mappedXor2 = premapper->map(xor2);

  EXPECT_TRUE(checker.areEquivalent(xor1->make(), mappedXor1->make()).equal());
  EXPECT_TRUE(checker.areEquivalent(xor2->make(), mappedXor2->make()).equal());
}

static void checkRandomSubnet(const SubnetMapper &premapper) {
  const uint16_t nIn      = 10u;
  const uint16_t nOut     = 1u;
  const uint32_t nCell    = 60u;
  const uint16_t MinArity = 1u;
  const uint16_t MaxArity = 6u;
  const uint16_t nLoops   = 20u;

  for (uint16_t i = 0; i < nLoops; ++i) {
    const auto id = eda::gate::model::randomSubnet(nIn, nOut, nCell,
                                                   MinArity, MaxArity);

    const auto &oldSubnet = Subnet::get(id);
    BuilderPtr builder = std::make_shared<Builder>(id);
    const auto mapped = premapper->map(builder);
    const auto &transformedSubnet = Subnet::get(mapped->make());

    EXPECT_EQ(eda::gate::model::evaluate(oldSubnet),
              eda::gate::model::evaluate(transformedSubnet));
  }
}

TEST(CellPremapperTest, SimpleCases) {
  const auto aigmapper = getCellAigMapper();
  const auto migmapper = getCellMigMapper();
  const auto xagmapper = getCellXagMapper();
  const auto xmgmapper = getCellXmgMapper();
  checkSimpleCases(aigmapper);
  checkSimpleCases(migmapper);
  checkSimpleCases(xagmapper);
  checkSimpleCases(xmgmapper);
}

TEST(CellPremapperTest, ConstCases) {
  const auto aigmapper = getCellAigMapper();
  const auto migmapper = getCellMigMapper();
  const auto xagmapper = getCellXagMapper();
  const auto xmgmapper = getCellXmgMapper();
  checkConstCases(aigmapper);
  checkConstCases(migmapper);
  checkConstCases(xagmapper);
  checkConstCases(xmgmapper);
}

TEST(CellPremapperTest, MAJ) {
  const auto aigmapper = getCellAigMapper();
  const auto migmapper = getCellMigMapper();
  const auto xagmapper = getCellXagMapper();
  const auto xmgmapper = getCellXmgMapper();
  checkMAJ(aigmapper);
  checkMAJ(migmapper);
  checkMAJ(xagmapper);
  checkMAJ(xmgmapper);
}

TEST(CellPremapperTest, AND) {
  const auto aigmapper = getCellAigMapper();
  const auto migmapper = getCellMigMapper();
  const auto xagmapper = getCellXagMapper();
  const auto xmgmapper = getCellXmgMapper();
  checkAND(aigmapper);
  checkAND(migmapper);
  checkAND(xagmapper);
  checkAND(xmgmapper);
}

TEST(CellPremapperTest, OR) {
  const auto aigmapper = getCellAigMapper();
  const auto migmapper = getCellMigMapper();
  const auto xagmapper = getCellXagMapper();
  const auto xmgmapper = getCellXmgMapper();
  checkOR(aigmapper);
  checkOR(migmapper);
  checkOR(xagmapper);
  checkOR(xmgmapper);
}

TEST(CellPremapperTest, XOR) {
  const auto aigmapper = getCellAigMapper();
  const auto migmapper = getCellMigMapper();
  const auto xagmapper = getCellXagMapper();
  const auto xmgmapper = getCellXmgMapper();
  checkXOR(aigmapper);
  checkXOR(migmapper);
  checkXOR(xagmapper);
  checkXOR(xmgmapper);
}

TEST(CellPremapperTest, RandomSubnet) {
  const auto aigmapper = getCellAigMapper();
  const auto migmapper = getCellMigMapper();
  const auto xagmapper = getCellXagMapper();
  const auto xmgmapper = getCellXmgMapper();
  checkRandomSubnet(aigmapper);
  checkRandomSubnet(migmapper);
  checkRandomSubnet(xagmapper);
  checkRandomSubnet(xmgmapper);
}

} // namespace eda::gate::premapper
