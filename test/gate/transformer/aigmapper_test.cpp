//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/celltype.h"
#include "gate/model2/subnet.h"
#include "gate/model2/utils/subnet_random.h"
#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/transformer/aigmapper.h"

#include "gtest/gtest.h"

#include <iostream>
#include <vector>

namespace eda::gate::transformer {
using AigMapper  = eda::gate::transformer::AigMapper;
using Builder    = eda::gate::model::SubnetBuilder;
using CellSymbol = eda::gate::model::CellSymbol;
using Link       = eda::gate::model::Subnet::Link;
using LinkList   = eda::gate::model::Subnet::LinkList;
using Subnet     = eda::gate::model::Subnet;
using SubnetID   = eda::gate::model::SubnetID;

SubnetID createPrimitiveSubnet(CellSymbol symbol, size_t nIn, size_t arity) {
  Builder builder;
  LinkList links;

  for (size_t i = 0; i < nIn; i++) {
    const auto idx = builder.addCell(CellSymbol::IN, Builder::INPUT);
    links.emplace_back(idx);
  }

  const auto idx = builder.addCellTree(symbol, links, arity);
  builder.addCell(CellSymbol::OUT, Link(idx), Builder::OUTPUT);

  return builder.make();
}

TEST(AigTransformer, MAJ) {
  const size_t nIn = 3;
  Builder builder;
  LinkList links;

  std::vector<size_t> idx;
  for (size_t i = 0; i < nIn; ++i) {
    idx.push_back(builder.addCell(CellSymbol::IN, Builder::INPUT));
  }

  for (size_t i = 0; i < nIn; ++i) {
    links.emplace_back(idx[i], i % 2);
  }

  idx.push_back(builder.addCell(CellSymbol::MAJ, links));
  builder.addCell(CellSymbol::OUT, idx.back(), Builder::OUTPUT);
  const auto id = builder.make();

  const auto &oldSubnet = Subnet::get(id);

  AigMapper mapper;
  const auto transformed = mapper.transform(id);

  const auto &transformedSubnet = Subnet::get(transformed);

  EXPECT_EQ(eda::gate::model::evaluate(oldSubnet),
            eda::gate::model::evaluate(transformedSubnet));
}

TEST(AigTransformer, AND) {
  const auto andSub  = createPrimitiveSubnet(CellSymbol::AND, 13, 13);
  const auto andTree = createPrimitiveSubnet(CellSymbol::AND, 13, 3);

  AigMapper mapper;
  const auto transformedAndSub  = mapper.transform(andSub);
  const auto transformedAndTree = mapper.transform(andTree);

  EXPECT_EQ(eda::gate::model::evaluate(Subnet::get(andSub)),
            eda::gate::model::evaluate(Subnet::get(transformedAndSub)));
  
  EXPECT_EQ(eda::gate::model::evaluate(Subnet::get(andTree)),
            eda::gate::model::evaluate(Subnet::get(transformedAndTree)));
}

TEST(AigTransformer, OR) {
  const auto orSub  = createPrimitiveSubnet(CellSymbol::OR, 13, 13);
  const auto orTree = createPrimitiveSubnet(CellSymbol::OR, 13, 5);

  AigMapper mapper;
  const auto transformedOrSub  = mapper.transform(orSub);
  const auto transformedOrTree = mapper.transform(orTree);

  EXPECT_EQ(eda::gate::model::evaluate(Subnet::get(orSub)),
            eda::gate::model::evaluate(Subnet::get(transformedOrSub)));
  
  EXPECT_EQ(eda::gate::model::evaluate(Subnet::get(orTree)),
            eda::gate::model::evaluate(Subnet::get(transformedOrTree)));
}

TEST(AigTransformer, XOR) {
  const auto xorSub  = createPrimitiveSubnet(CellSymbol::XOR, 13, 13);
  const auto xorTree = createPrimitiveSubnet(CellSymbol::XOR, 13, 4);

  AigMapper mapper;
  const auto transformedXorSub  = mapper.transform(xorSub);
  const auto transformedXorTree = mapper.transform(xorTree);

  EXPECT_EQ(eda::gate::model::evaluate(Subnet::get(xorSub)),
            eda::gate::model::evaluate(Subnet::get(transformedXorSub)));
  
  EXPECT_EQ(eda::gate::model::evaluate(Subnet::get(xorTree)),
            eda::gate::model::evaluate(Subnet::get(transformedXorTree)));
}

/// TODO: The test doesn't work properly. Need issue?
/*
TEST(AigTransformer, RandomSubnet) {
  using AigMapper = eda::gate::transformer::AigMapper;
  using Subnet    = eda::gate::model::Subnet;

  const size_t nIn      = 3u;
  const size_t nOut     = 1u;
  const size_t nCell    = 5u;
  const size_t MinArity = 1u;
  const size_t MaxArity = 3u;

  const auto id = eda::gate::model::randomSubnet(nIn, nOut, nCell,
                                                 MinArity, MaxArity);

  const auto &oldSubnet = Subnet::get(id);
  std::cout << oldSubnet << std::endl;

  AigMapper mapper;
  const auto transformed = mapper.transform(id);

  std::cout << "Transformed subnet:" << std::endl;

  const auto &transformedSubnet = Subnet::get(transformed);
  std::cout << transformedSubnet << std::endl;

  EXPECT_EQ(eda::gate::model::evaluate(oldSubnet),
            eda::gate::model::evaluate(transformedSubnet));
}
*/

} // namespace eda::gate::transformer