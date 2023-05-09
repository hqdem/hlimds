//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/checker.h"
#include "gate/model/gnet_test.h"

#include "gate/premapper/xmgmapper.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using Checker = eda::gate::debugger::Checker;
using GateBinding = Checker::GateBinding;
using GateIdMap = eda::gate::premapper::XmgMapper::GateIdMap;
using Hints = Checker::Hints;
using Link = eda::gate::model::Gate::Link;
using XmgMapper = eda::gate::premapper::XmgMapper;

void initializeXmgBinds(const GNet &net,
                        GateIdMap &gmap,
                        GateBinding &ibind,
                        GateBinding &obind) {
  // Input-to-input correspondence.
  for (const auto oldSourceLink : net.sourceLinks()) {
    auto newSourceId = gmap[oldSourceLink.target];
    ibind.insert({oldSourceLink, Link(newSourceId)});
  }

  // Output-to-output correspondence.
  for (const auto oldTargetLink : net.targetLinks()) {
    auto newTargetId = gmap[oldTargetLink.source];
    obind.insert({oldTargetLink, Link(newTargetId)});
  }
}

std::shared_ptr<GNet> xmgMap(std::shared_ptr<GNet> net, GateIdMap &gmap) {
  eda::gate::premapper::XmgMapper xmgMapper;
  std::shared_ptr<GNet> xmgMapped = xmgMapper.map(*net, gmap);
  //dump(*net);
  //dump(*migMapped);
  xmgMapped->sortTopologically();
  return xmgMapped;
}

bool checkXmgEquivalence(const std::shared_ptr<GNet> net,
                         const std::shared_ptr<GNet> xmgMapped,
                         GateIdMap &gmap) {
  // Initialize binds
  GateBinding ibind, obind;
  initializeXmgBinds(*net, gmap, ibind, obind);
  eda::gate::debugger::Checker::Hints hints;
  hints.sourceBinding  = std::make_shared<GateBinding>(std::move(ibind));
  hints.targetBinding  = std::make_shared<GateBinding>(std::move(obind));
  // check equivalence
  eda::gate::debugger::Checker checker;
  bool equal = checker.areEqual(*net, *xmgMapped, hints);
  return equal;
}

TEST(XmgMapperTest, XmgMapperOrTest) {
  Gate::SignalList inputs;
  Gate::Id outputId;
  auto net = makeOr(1024, inputs, outputId);
  GateIdMap gmap;
  std::shared_ptr<GNet> xmgMapped = xmgMap(net, gmap);
  EXPECT_TRUE(checkXmgEquivalence(net, xmgMapped, gmap));
}

TEST(XmgMapperTest, XmgMapperAndTest) {
  Gate::SignalList inputs;
  Gate::Id outputId;
  auto net = makeAnd(1024, inputs, outputId);
  GateIdMap gmap;
  std::shared_ptr<GNet> xmgMapped = xmgMap(net, gmap);
  EXPECT_TRUE(checkXmgEquivalence(net, xmgMapped, gmap));
}

TEST(XmgMapperTest, XmgMapperMajOf3Test) {
  Gate::SignalList inputs;
  Gate::Id outputId;
  auto net = makeMaj(3, inputs, outputId);
  GateIdMap gmap;
  std::shared_ptr<GNet> xmgMapped = xmgMap(net, gmap);
  EXPECT_TRUE(checkXmgEquivalence(net, xmgMapped, gmap));
}

TEST(XmgMapperTest, XmgMapperMajOf5Test) {
  Gate::SignalList inputs;
  Gate::Id outputId;
  auto net = makeMaj(5, inputs, outputId);
  GateIdMap gmap;
  std::shared_ptr<GNet> xmgMapped = xmgMap(net, gmap);
  EXPECT_TRUE(checkXmgEquivalence(net, xmgMapped, gmap));
}

TEST(XmgMapperTest, XmgMapperNorTest) {
  Gate::SignalList inputs;
  Gate::Id outputId;
  auto net = makeNor(1024, inputs, outputId);
  GateIdMap gmap;
  std::shared_ptr<GNet> xmgMapped = xmgMap(net, gmap);
  EXPECT_TRUE(checkXmgEquivalence(net, xmgMapped, gmap));
}

TEST(XmgMapperTest, XmgMapperNandTest) {
  Gate::SignalList inputs;
  Gate::Id outputId;
  auto net = makeNand(1024, inputs, outputId);
  GateIdMap gmap;
  std::shared_ptr<GNet> xmgMapped = xmgMap(net, gmap);
  EXPECT_TRUE(checkXmgEquivalence(net, xmgMapped, gmap));
}

TEST(XmgMapperTest, XmgMapperOrnTest) {
  Gate::SignalList inputs;
  Gate::Id outputId;
  auto net = makeOrn(1024, inputs, outputId);
  GateIdMap gmap;
  std::shared_ptr<GNet> xmgMapped = xmgMap(net, gmap);
  EXPECT_TRUE(checkXmgEquivalence(net, xmgMapped, gmap));
}

TEST(XmgMapperTest, XmgMapperAndnTest) {
  Gate::SignalList inputs;
  Gate::Id outputId;
  auto net = makeAndn(1024, inputs, outputId);
  GateIdMap gmap;
  std::shared_ptr<GNet> xmgMapped = xmgMap(net, gmap);
  EXPECT_TRUE(checkXmgEquivalence(net, xmgMapped, gmap));
}
