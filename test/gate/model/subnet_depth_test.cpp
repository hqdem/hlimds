//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnet.h"

#include "gtest/gtest.h"

#include <vector>

namespace eda::gate::model {

inline void checkDepth(const Subnet &subnet,
                       const std::vector<size_t> &correctDepth) {
  const auto &entries = subnet.getEntries();
  std::vector<size_t> depth(entries.size(), 0);
  for (size_t i = 0; i < entries.size(); ++i) {
    for (const auto &link : subnet.getLinks(i)) {
      depth[i] = std::max(depth[i], depth[link.idx] + 1);
    }
    EXPECT_EQ(depth[i], correctDepth[i]);
    i += entries[i].cell.more;
  }
}

TEST(SubnetDepthTest, SimpleTest) {
  SubnetBuilder builder;

  Subnet::LinkList inputs = builder.addInputs(2);

  Subnet::Link link1 = builder.addCell(AND, inputs[0], inputs[1]);
  Subnet::Link link2 = builder.addCell(OR, ~inputs[0], ~inputs[1]);
  Subnet::Link link3 = builder.addCell(OR, link1, link2);
  Subnet::Link link4 = builder.addCell(XOR, ~inputs[0], ~inputs[1]);

  builder.addOutput(inputs[0]);
  builder.addOutput(link4);
  builder.addOutput(link3);

  const auto &result = Subnet::get(builder.make());
  checkDepth(result, {0, 0, 1, 1, 1, 2, 1, 2, 3});
}

TEST(SubnetDepthTest, LinkEntriesTest1) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(8);
  Subnet::LinkList andInputs0{inLinks[0], inLinks[1], inLinks[2], inLinks[3],
                              inLinks[4], inLinks[5]};
  const auto &andLink0 = builder.addCell(model::AND, andInputs0);
  Subnet::LinkList orInputs0{andLink0, inLinks[3], inLinks[4], inLinks[5],
                             inLinks[6], inLinks[7]};
  const auto &orLink0 = builder.addCell(model::OR, orInputs0);
  builder.addOutput(orLink0);
  Subnet::LinkList xorInputs0{andLink0, inLinks[0], inLinks[1], inLinks[2],
                              inLinks[3], inLinks[4], inLinks[5], inLinks[6],
                              inLinks[7], orLink0};
  const auto &xorLink0 = builder.addCell(model::XOR, xorInputs0);
  builder.addOutput(xorLink0);
  const auto &result = Subnet::get(builder.make());

  checkDepth(result, {0, 0, 0, 0, 0, 0, 0, 0, 1, size_t(-1), 2, size_t(-1), 3,
                      size_t(-1), 3, 4});
}

TEST(SubnetDepthTest, LinkEntriesTest2) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(6);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1],
                                         inLinks[2]);
  const auto &orLink0 = builder.addCell(model::OR, inLinks[3], inLinks[4],
                                        inLinks[5]);
  const auto &xorLink0 = builder.addCell(model::XOR, andLink0, inLinks[1]);
  const Subnet::LinkList xorInputs1{inLinks[0], inLinks[1], inLinks[2],
                                    inLinks[3], inLinks[4], andLink0, orLink0};
  const auto &xorLink1 = builder.addCell(model::XOR, xorInputs1);
  builder.addOutput(xorLink0);
  builder.addOutput(xorLink1);
  const auto &result = Subnet::get(builder.make());

  checkDepth(result, {0, 0, 0, 0, 0, 0, 1, 1, 2, 2, size_t(-1), 3, 3});
}

TEST(SubnetDepthTest, LinkEntriesTest3) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(14);
  const auto &orLink0 = builder.addCell(model::OR, inLinks[3], inLinks[4],
                                        inLinks[5]);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1],
                                         inLinks[2]);
  const Subnet::LinkList orInputs1{inLinks[0], inLinks[1], inLinks[2],
                                   inLinks[3], inLinks[4], inLinks[5],
                                   inLinks[6], inLinks[7], inLinks[8],
                                   inLinks[9], inLinks[10], inLinks[11],
                                   orLink0, andLink0};
  const auto &orLink1 = builder.addCell(model::OR, orInputs1);
  const Subnet::LinkList xorInputs0{inLinks[0], inLinks[1], inLinks[2],
                                    inLinks[3], inLinks[4], inLinks[5],
                                    inLinks[6], inLinks[7], inLinks[8],
                                    inLinks[9], inLinks[10], inLinks[11],
                                    andLink0, orLink0};
  const auto &xorLink0 = builder.addCell(model::XOR, xorInputs0);
  builder.addOutput(xorLink0);
  builder.addOutput(orLink1);
  const auto &result = Subnet::get(builder.make());

  checkDepth(result, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2,
                      size_t(-1), size_t(-1), 2, size_t(-1), size_t(-1), 3, 3});
}

TEST(SubnetDepthTest, ReuseCellsFollowingRoot) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(3);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &andLink1 = builder.addCell(model::AND, inLinks[1], inLinks[2]);
  const auto &andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  builder.addOutput(andLink2);
  const auto &orLink0 = builder.addCell(model::OR, inLinks[0], inLinks[1]);
  const auto &orLink1 = builder.addCell(model::OR, orLink0, inLinks[2]);
  const auto &orLink2 = builder.addCell(model::OR, orLink1, inLinks[2]);
  builder.addOutput(orLink2);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(3);
  const auto &rhsOrLink0 = rhsBuilder.addCell(model::OR, rhsInLinks[0],
                                              rhsInLinks[1]);
  const auto &rhsOrLink1 = rhsBuilder.addCell(model::OR, rhsOrLink0,
                                              rhsInLinks[2]);
  const auto &rhsOrLink2 = rhsBuilder.addCell(model::OR, rhsOrLink1,
                                              rhsInLinks[2]);
  const auto &rhsBufLink0 = rhsBuilder.addCell(model::BUF, rhsOrLink2);
  rhsBuilder.addOutput(rhsBufLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({0, 1, 2}, {5});

  builder.replace(rhsID, mapping);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);

  checkDepth(result, {0, 0, 0, 1, 2, 3, 4, 5, 4});
}

TEST(SubnetDepthTest, ReduceRootDepth) {
  SubnetBuilder builder;
  const auto &inLinks = builder.addInputs(3);
  const auto &andLink0 = builder.addCell(model::AND, inLinks[0], inLinks[1]);
  const auto &andLink1 = builder.addCell(model::AND, inLinks[1], inLinks[2]);
  const auto &andLink2 = builder.addCell(model::AND, andLink0, andLink1);
  const auto &orLink0 = builder.addCell(model::OR, inLinks[0], andLink2);
  const auto &orLink1 = builder.addCell(model::OR, orLink0, inLinks[2]);
  const auto &bufLink0 = builder.addCell(model::BUF, orLink1);
  builder.addOutput(bufLink0);

  SubnetBuilder rhsBuilder;
  const auto &rhsInLinks = rhsBuilder.addInputs(3);
  const auto &rhsOrLink0 = rhsBuilder.addCell(model::OR, rhsInLinks[0],
                                              rhsInLinks[1]);
  rhsBuilder.addOutput(rhsOrLink0);

  const auto rhsID = rhsBuilder.make();
  InOutMapping mapping({0, 1, 2}, {5});

  builder.replace(rhsID, mapping);

  const SubnetID resultID = builder.make();
  const Subnet &result = Subnet::get(resultID);

  checkDepth(result, {0, 0, 0, 1, 2, 3, 4, 5});
}

} // namespace eda::gate::model
