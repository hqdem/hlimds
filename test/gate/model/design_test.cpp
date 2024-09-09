//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/sat_checker.h"
#include "gate/model/design.h"
#include "gate/model/examples.h"
#include "gate/model/net.h"
#include "gate/model/printer/dataflow_printer.h"
#include "gate/optimizer/pass.h"
#include "test_util.h"

#include "gtest/gtest.h"

#include <cstddef>

namespace eda::gate::model {

using namespace eda::gate::debugger;
using namespace eda::gate::optimizer;

using SignalType = NetDecomposer::SignalType;
using NetBuilder = model::NetBuilder;

struct Arc final {
  size_t connectedSubnet;
  NetDecomposer::ConnectionDesc connectionDesc;
};

struct SubnetArcs final {
  std::vector<Arc> arcs;
  bool connectedPI{false};
  bool connectedPO{false};
  NetDecomposer::ConnectionDesc PIArcDesc;
  NetDecomposer::ConnectionDesc POArcDesc;
};

static const std::string designOutPath =
    "test/gate/model/design/";

static void test(const std::shared_ptr<DesignBuilder> &builder) {
  EXPECT_TRUE(builder->getSubnetNum() != 0);

  foreach(aig())->transform(builder);
  builder->save("premapped");

  foreach(resyn())->transform(builder);
  builder->save("optimized");

  const auto &checker = SatChecker::get();
  const auto result = checker.areEquivalent(*builder, "premapped", "optimized");
  EXPECT_TRUE(result.equal());

  const auto premappedNetID = builder->make("premapped");
  EXPECT_TRUE(premappedNetID != OBJ_NULL_ID);

  const auto optimizedNetID = builder->make("optimized");
  EXPECT_TRUE(optimizedNetID != OBJ_NULL_ID);

#ifdef UTOPIA_DEBUG
  std::cout << Net::get(premappedNetID) << std::endl;
  std::cout << Net::get(optimizedNetID) << std::endl;
#endif // UTOPIA_DEUB
}

static bool descsEqual(
    const NetDecomposer::ConnectionDesc &lhs,
    const NetDecomposer::ConnectionDesc &rhs) {
  return lhs.signalType == rhs.signalType;
}

static bool inOutArcsEqual(
    const DesignBuilder::SubnetEntry &entry,
    const SubnetArcs &correctArcs) {
  if (entry.hasPIArc() != correctArcs.connectedPI ||
      entry.hasPOArc() != correctArcs.connectedPO) {
    return false;
  }
  if ((entry.hasPIArc() &&
       !descsEqual(correctArcs.PIArcDesc, entry.getPIArcDesc())) ||
      (entry.hasPOArc() &&
       !descsEqual(correctArcs.POArcDesc, entry.getPOArcDesc()))) {
    return false;
  }
  return true;
}

static bool subnetArcsEqual(
    const std::shared_ptr<DesignBuilder> &builder,
    const std::vector<SubnetArcs> &correctSubnetArcs) {

  if (correctSubnetArcs.size() != builder->getSubnetNum()) {
    return false;
  }
  for (size_t i = 0; i < correctSubnetArcs.size(); ++i) {
    const auto &entry = builder->getEntry(i);
    const auto &entryArcs = entry.getInArcs();
    const auto correctArcs = correctSubnetArcs[i].arcs;
    if (entryArcs.size() != correctArcs.size()) {
      return false;
    }
    for (const auto &arc : correctArcs) {
      const size_t arcSource = arc.connectedSubnet;
      const NetDecomposer::ConnectionDesc &correctDesc = arc.connectionDesc;
      if (entryArcs.find(arcSource) == entryArcs.end() ||
          !descsEqual(correctDesc, entry.getArcDesc(arcSource))) {
        return false;
      }
    }
    if (!inOutArcsEqual(entry, correctSubnetArcs[i])) {
      return false;
    }
  }
  return true;
}

static void testArcDescs(
    const std::shared_ptr<DesignBuilder> &builder,
    const std::vector<SubnetArcs> &correctSubnetArcs) {
  EXPECT_TRUE(subnetArcsEqual(builder, correctSubnetArcs));
}

void printDesign(
    const std::shared_ptr<DesignBuilder> &builder,
    const std::string &name) {
  std::ofstream out;
  std::filesystem::path filePath = createOutDir(designOutPath);
  out.open(filePath.c_str() + name + ".dot");
  out << *builder << '\n';
  out.close();
}

TEST(DesignTest, OneTriggerNet) {
  const auto &in1 = makeCell(IN);
  const auto &in2 = makeCell(IN);
  const auto &in3 = makeCell(IN);
  const auto &dff1 = makeCell(DFF_p, in1, in2, in3);
  const auto &out1 = makeCell(OUT, dff1);
  const auto &out2 = makeCell(OUT, in1);

  NetBuilder netBuilder;
  netBuilder.addCell(in1);
  netBuilder.addCell(in2);
  netBuilder.addCell(in3);
  netBuilder.addCell(dff1);
  netBuilder.addCell(out1);
  netBuilder.addCell(out2);

  const NetID netID = netBuilder.make();

  auto builder = std::make_shared<DesignBuilder>(netID);
  test(builder);
  printDesign(builder, "one_trigger_net");
}

TEST(DesignTest, RandomSubnet) {
  const size_t nIn = 10;
  const size_t nOut = 10;
  const size_t nCell = 30;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const unsigned seed = 0;

  const auto subnetID = makeSubnetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  auto builder = std::make_shared<DesignBuilder>(subnetID);
  test(builder);
  printDesign(builder, "random_subnet");
}

TEST(DesignTest, RandomNet) {
  const size_t nIn = 10;
  const size_t nOut = 10;
  const size_t nCell = 30;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const unsigned seed = 0;

  const auto netID = makeNetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  auto builder = std::make_shared<DesignBuilder>(netID);
  test(builder);
  printDesign(builder, "random_net");
}

TEST(DesignTest, Print100) {
  const size_t nIn = 50;
  const size_t nOut = 50;
  const size_t nCell = 100;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const unsigned seed = 0;

  const auto netID = makeTriggerNetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  auto builder = std::make_shared<DesignBuilder>(netID);
  printDesign(builder, "100_elements");
}

TEST(DesignTest, Print5000) {
  const size_t nIn = 1000;
  const size_t nOut = 1000;
  const size_t nCell = 5000;
  const size_t minArity = 2;
  const size_t maxArity = 3;
  const unsigned seed = 0;

  const auto netID = makeTriggerNetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  auto builder = std::make_shared<DesignBuilder>(netID);
  printDesign(builder, "5000_elements");
}

TEST(DesignTest, ArcDescOneDff) {
  const auto &in1 = makeCell(IN);
  const auto &in2 = makeCell(IN);
  const auto &in3 = makeCell(IN);
  const auto &dff1 = makeCell(DFF_p, in1, in2, in3);
  const auto &out1 = makeCell(OUT, dff1);

  NetBuilder netBuilder;
  netBuilder.addCell(in1);
  netBuilder.addCell(in2);
  netBuilder.addCell(in3);
  netBuilder.addCell(dff1);
  netBuilder.addCell(out1);

  const NetID netID = netBuilder.make();

  auto builder = std::make_shared<DesignBuilder>(netID);
  testArcDescs(builder, {
    { {{1, SignalType::DATA}, {2, SignalType::CLOCK}, {3, SignalType::RESET}},
      false, true, {}, {SignalType::DATA}
    },
    { {}, true, false, {SignalType::DATA}, {} },
    { {}, true, false, {SignalType::CLOCK}, {} },
    { {}, true, false, {SignalType::RESET}, {} },
  });
}

TEST(DesignTest, ArcDescOneDLatchRs) {
  const auto &in1 = makeCell(IN);
  const auto &in2 = makeCell(IN);
  const auto &in3 = makeCell(IN);
  const auto &in4 = makeCell(IN);
  const auto &dLatchRs1 = makeCell(DLATCHrs_ppp, in1, in2, in3, in4);
  const auto &out1 = makeCell(OUT, dLatchRs1);

  NetBuilder netBuilder;
  netBuilder.addCell(in1);
  netBuilder.addCell(in2);
  netBuilder.addCell(dLatchRs1);
  netBuilder.addCell(out1);

  const NetID netID = netBuilder.make();

  auto builder = std::make_shared<DesignBuilder>(netID);
  testArcDescs(builder, {
    { {{1, SignalType::DATA}, {2, SignalType::ENABLE},
       {3, SignalType::RESET}, {4, SignalType::SET}},
       false, true, {}, {SignalType::DATA}
    },
    { {}, true, false, {SignalType::DATA}, {} },
    { {}, true, false, {SignalType::ENABLE}, {} },
    { {}, true, false, {SignalType::RESET}, {} },
    { {}, true, false, {SignalType::SET}, {} },
  });
}

TEST(DesignTest, ArcDescOneLatchrs) {
  const auto &in1 = makeCell(IN);
  const auto &in2 = makeCell(IN);
  const auto &latchRs1 = makeCell(LATCHrs_pp, in1, in2);
  const auto &out1 = makeCell(OUT, latchRs1);

  NetBuilder netBuilder;
  netBuilder.addCell(in1);
  netBuilder.addCell(in2);
  netBuilder.addCell(latchRs1);
  netBuilder.addCell(out1);

  const NetID netID = netBuilder.make();

  auto builder = std::make_shared<DesignBuilder>(netID);
  testArcDescs(builder, {
    { {{1, SignalType::RESET}, {2, SignalType::SET}}, false, true, {},
      {SignalType::DATA}
    },
    { {}, true, false, {SignalType::RESET}, {} },
    { {}, true, false, {SignalType::SET}, {} },
  });
}

TEST(DesignTest, ArcDescDffChain) {
  const auto &in1 = makeCell(IN);
  const auto &in2 = makeCell(IN);
  const auto &in3 = makeCell(IN);
  const auto &in4 = makeCell(IN);
  const auto &in5 = makeCell(IN);
  const auto &dff1 = makeCell(DFF_p, in1, in2, in3);
  const auto &dff2 = makeCell(DFF_p, dff1, in2, in3);
  const auto &dff3 = makeCell(DFF_p, dff2, in4, in5);
  const auto &out1 = makeCell(OUT, dff3);

  NetBuilder netBuilder;
  netBuilder.addCell(in1);
  netBuilder.addCell(in2);
  netBuilder.addCell(in3);
  netBuilder.addCell(in4);
  netBuilder.addCell(in5);
  netBuilder.addCell(dff1);
  netBuilder.addCell(dff2);
  netBuilder.addCell(dff3);
  netBuilder.addCell(out1);

  const NetID netID = netBuilder.make();

  auto builder = std::make_shared<DesignBuilder>(netID);
  testArcDescs(builder, {
    { {{7, SignalType::DATA}, {8, SignalType::CLOCK}, {9, SignalType::RESET}},
      false, true, {}, {SignalType::DATA}
    },
    { {}, true, false, {SignalType::DATA}, {} },
    { {}, true, false, {SignalType::CLOCK}, {} },
    { {}, true, false, {SignalType::RESET}, {} },
    { {{1, SignalType::DATA}, {2, SignalType::CLOCK}, {3, SignalType::RESET}},
      false, false, {}, {}
    },
    { {}, true, false, {SignalType::CLOCK}, {} },
    { {}, true, false, {SignalType::RESET}, {} },
    { {{4, SignalType::DATA}, {5, SignalType::CLOCK}, {6, SignalType::RESET}},
      false, false, {}, {}
    },
    { {}, true, false, {SignalType::CLOCK}, {} },
    { {}, true, false, {SignalType::RESET}, {} },
  });
}

TEST(DesignTest, ArcDescLatchRsChain) {
  const auto &in1 = makeCell(IN);
  const auto &in2 = makeCell(IN);
  const auto &in3 = makeCell(IN);
  const auto &latchrs1 = makeCell(LATCHrs_pp, in1, in2);
  const auto &latchrs2 = makeCell(LATCHrs_pp, latchrs1, in2);
  const auto &latchrs3 = makeCell(LATCHrs_pp, latchrs2, in3);
  const auto &out1 = makeCell(OUT, latchrs3);

  NetBuilder netBuilder;
  netBuilder.addCell(in1);
  netBuilder.addCell(in2);
  netBuilder.addCell(in3);
  netBuilder.addCell(latchrs1);
  netBuilder.addCell(latchrs2);
  netBuilder.addCell(latchrs3);
  netBuilder.addCell(out1);

  const NetID netID = netBuilder.make();

  auto builder = std::make_shared<DesignBuilder>(netID);
  testArcDescs(builder, {
    { {{5, SignalType::RESET}, {6, SignalType::SET}}, false, true, {},
      {SignalType::DATA}
    },
    { {}, true, false, {SignalType::RESET}, {} },
    { {}, true, false, {SignalType::SET}, {} },
    { {{1, SignalType::RESET}, {2, SignalType::SET}}, false, false, {}, {} },
    { {}, true, false, {SignalType::SET}, {} },
    { {{3, SignalType::RESET}, {4, SignalType::SET}}, false, false, {}, {} },
    { {}, true, false, {SignalType::SET}, {} },
  });
}

TEST(DesignTest, ArcDescInOut) {
  const auto &in1 = makeCell(IN);
  const auto &out1 = makeCell(OUT, in1);

  NetBuilder netBuilder;
  netBuilder.addCell(in1);
  netBuilder.addCell(out1);

  const NetID netID = netBuilder.make();

  auto builder = std::make_shared<DesignBuilder>(netID);
  testArcDescs(builder, {
    { {}, true, true, {SignalType::DATA}, {SignalType::DATA} },
  });
}

TEST(DesignTest, ArcDescInOutSubnet) {
  SubnetBuilder subnetBuilder;
  const auto &in1 = subnetBuilder.addInput();
  subnetBuilder.addOutput(in1);
  const auto subnetID = subnetBuilder.make();

  auto builder = std::make_shared<DesignBuilder>(subnetID);
  testArcDescs(builder, {
    { {}, true, true, {SignalType::DATA}, {SignalType::DATA} },
  });
}

} // namespace eda::gate::model
