//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/check_cut.h"
#include "gate/optimizer/optimizer.h"
#include "gate/parser/gate_verilog_parser.h"
#include "gtest/gtest.h"
#include "gate/printer/dot.h"
#include "gate/optimizer/cone_visitor.h"

#include <lorina/diagnostics.hpp>
#include <lorina/verilog.hpp>

#include <filesystem>
#include <string>

namespace eda::gate::optimizer {
  using Simulator = eda::gate::simulator::Simulator;

  GateID createLink(GNet &gNet, const std::vector<GateID> &g,
                    const std::vector<GateID> &input,
                    model::GateSymbol func = model::GateSymbol::Value::AND) {
    std::vector<base::model::Signal<GNet::GateId>> signals;
    for (GateID gate: input) {
      signals.emplace_back(base::model::Event::ALWAYS, g[gate]);
    }
    return gNet.addGate(func, signals);
  }

  static std::vector<GateID> gnet1(GNet &gNet) {
    std::vector<GateID> g(4);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}));
    g.push_back(createLink(gNet, g, {5, 3}));
    return g;
  }

  static std::vector<GateID> gnet1Exteded(GNet &gNet) {
    std::vector<GateID> g(4);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}));
    g.push_back(createLink(gNet, g, {5, 3}));
    g.push_back(createLink(gNet, g, {5, 6}, model::GateSymbol::Value::OUT));
    return g;
  }

  static std::vector<GateID> gnet2(GNet &gNet) {
    std::vector<GateID> g(4);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {3, 2}));
    g.push_back(createLink(gNet, g, {5, 4}));
    return g;
  }

  static std::vector<GateID> gnet3(GNet &gNet) {
    std::vector<GateID> g(7);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {0, 3, 7}));
    g.push_back(createLink(gNet, g, {2, 3, 4}));
    g.push_back(createLink(gNet, g, {5, 6, 7}));
    g.push_back(createLink(gNet, g, {5, 8}));
    g.push_back(createLink(gNet, g, {8, 7}));
    g.push_back(createLink(gNet, g, {9, 10}));
    g.push_back(createLink(gNet, g, {9, 7, 6}));
    g.push_back(createLink(gNet, g, {11, 12, 14}));
    return g;
  }

  GNet *findConePrint(const std::filesystem::path &subCatalog, GNet *net,
                      const std::vector<GNet::V> &cuNodes, GNet::V start) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath = homePath / subCatalog;

    system(std::string("mkdir -p ").append(outputPath).c_str());

    std::string filename1 = outputPath / "cone0.dot";
    std::string filename2 = outputPath / "cone.dot";

    Dot printer(net);
    printer.print(filename1);

    Cut cut;
    for (auto node: cuNodes) {
      cut.emplace(node);
    }

    ConeVisitor coneVisitor(cut);
    Walker walker(net, &coneVisitor, nullptr);
    walker.walk(start, false);

    GNet *subnet = coneVisitor.getGNet();

    printer = Dot(subnet);
    printer.print(filename2);

    return subnet;
  }

  void substitutePrint(const std::filesystem::path &subCatalog,
                       GNet *net, GNet *subNet, GateID cutFor,
                       const std::unordered_set<GNet::V> &cut) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath = homePath / subCatalog;

    system(std::string("mkdir -p ").append(outputPath).c_str());

    std::string filename1 = outputPath / "gnet1.dot";
    std::string filename2 = outputPath / "gnet2.dot";
    std::string filename12 = outputPath / "gnet12.dot";

    Dot mainGnetDot(net);
    Dot subGnetDot(subNet);

    mainGnetDot.print(filename1);
    subGnetDot.print(filename2);

    substitute(cutFor, cut, subNet, net);

    mainGnetDot.print(filename12);
  }

  void simulatorPrint(GNet *net, const std::vector<GNet::V> &in,
                      const std::vector<GNet::V> &out) {
    net->sortTopologically();

    GNet::LinkList inLinks, outLinks;

    for (auto node: in) {
      inLinks.emplace_back(Gate::Link(node));
    }

    for (auto node: out) {
      outLinks.emplace_back(Gate::Link(node));
    }
    Simulator simulator;

    auto compiled = simulator.compile(*net, inLinks, outLinks);

    std::uint64_t o;
    for (std::uint64_t i = 0; i < (1ull << inLinks.size()); i++) {
      compiled.simulate(o, i);
      std::cout << std::hex << i << " -> " << o << std::endl;
    }
  }

  void rewritePrint(const std::filesystem::path &subCatalog, GNet *net) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath = homePath / subCatalog;

    system(std::string("mkdir -p ").append(outputPath).c_str());

    std::string filenameBefore = outputPath / "gnet.dot";
    std::string filenameAfter = outputPath / "gnet_rewritten.dot";

    Dot dot(net);
    dot.print(filenameBefore);

    optimize(net, 4);

    dot.print(filenameAfter);
  }

// TODO: move std::vector<GateID> g to each test to provide correct mapping with node id.
//===----------------------------------------------------------------------===//
// Tests
//===----------------------------------------------------------------------===//

  TEST(OptimizerTest, optimizeToZeroNetGnet1) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet1(net);
    rewritePrint("test/data/gate/optimizer/output/rewrite1", &net);
  }

  TEST(OptimizerTest, optimizeToZeroNetGnet1Extended) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet1Exteded(net);
    rewritePrint("test/data/gate/optimizer/output/rewrite1ex", &net);
  }

  TEST(OptimizerTest, optimizeToZeroNetGnet2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet2(net);
    rewritePrint("test/data/gate/optimizer/output/rewrite2", &net);
  }

  TEST(OptimizerTest, optimizeToZeroNetGnet3) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet3(net);
    optimizePrint(&net, 4, "test/data/gate/optimizer/output/rewrite3");
  }

//===----------------------------------------------------------------------===//

  TEST(OptimizerTest, simulatorUse) {
    GNet net;

    auto g = gnet2(net);

    simulatorPrint(&net, {g[0], g[1], g[2], g[3]}, {g[6]});
  }

//===----------------------------------------------------------------------===//

  TEST(OptimizerTest, findCone) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1(net);

    auto *cone = findConePrint("test/data/gate/optimizer/output/findCone1",
                               &net, {g[2], g[4]}, g[5]);
    EXPECT_EQ(3, cone->nGates());
    delete cone;
  }

  TEST(OptimizerTest, findCone2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet3(net);

    auto cone = findConePrint("test/data/gate/optimizer/output/findCone2", &net,
                              {g[2], g[3], g[4], g[6], g[7]}, g[14]);
    EXPECT_EQ(7, cone->nGates());
    delete cone;
  }

  TEST(OptimizerTest, findCone3) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet3(net);

    auto cone = findConePrint("test/data/gate/optimizer/output/findCone3", &net,
                              {g[0], g[3], g[7]}, g[8]);
    EXPECT_EQ(4, cone->nGates());
    EXPECT_EQ(2, cone->nSourceLinks());
    delete cone;
  }

  TEST(OptimizerTest, findConeExessiveCut) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1(net);

    auto cone = findConePrint(
            "test/data/gate/optimizer/output/findConeExessiveCut", &net,
            {g[0], g[1], g[2], g[4]}, g[5]);
    EXPECT_EQ(3, cone->nGates());
    EXPECT_EQ(2, cone->nSourceLinks());
    delete cone;
  }

  TEST(OptimizerTest, findConeTrivial) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    auto g = gnet1(net);

    auto cone = findConePrint("test/data/gate/optimizer/output/findConeTrivial",
                              &net, {g[5]}, g[5]);
    EXPECT_EQ(1, cone->nGates());
    delete cone;
  }

//===----------------------------------------------------------------------===//

  TEST(OptimizerTest, substitute) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet mainGnet;
    GNet subGnet;

    auto g = gnet1(mainGnet);
    gnet2(subGnet);

    substitutePrint("test/data/gate/optimizer/output/substitute", &mainGnet,
                    &subGnet, g[6], mainGnet.getSources());
    EXPECT_EQ(mainGnet.nGates(), subGnet.nGates());
  }

  TEST(OptimizerTest, substitute2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet mainGnet;
    GNet subGnet;
    auto g = gnet1Exteded(mainGnet);
    gnet2(subGnet);

    substitutePrint("test/data/gate/optimizer/output/substitute2", &mainGnet,
                    &subGnet, g[6], mainGnet.getSources());
  }

//===----------------------------------------------------------------------===//

  TEST(OptimizerTest, FindCut_c17) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    const std::filesystem::path subCatalog = "test/data/gate/parser/verilog";
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path prefixPath = homePath / subCatalog;
    const std::string infile = "c17";

    std::string filename = prefixPath / (infile + ".v");
    lorina::text_diagnostics consumer;
    lorina::diagnostic_engine diag(&consumer);

    GateVerilogParser parser(infile);
    assert(lorina::return_code::success ==
           read_verilog(filename, parser, &diag));

    CutStorage storage = findCuts(4, parser.getGnet());

    for (auto &[v, cs]: storage.cuts) {
      for (const auto &c: cs) {
        GateID failed;

        if (!isCut(v, c, failed)) {
          FAIL() << "Wrong cut for v " << v << "; failed " << failed << "\n";
        }
      }
    }

    delete parser.getGnet();
  }
}
