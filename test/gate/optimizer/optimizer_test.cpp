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
  using GNet = eda::gate::model::GNet;
  using Vertex = eda::gate::model::GNet::V;
  using Simulator = eda::gate::simulator::Simulator;

  Vertex createLink(eda::gate::model::GNet &gNet, const std::vector<Vertex> &g,
                    const std::vector<Vertex> &input,
                    eda::gate::model::GateSymbol func = eda::gate::model::GateSymbol::Value::AND) {
    std::vector<eda::base::model::Signal<GNet::GateId>> signals;
    for (Vertex gate: input) {
      signals.emplace_back(eda::base::model::Event::ALWAYS, g[gate]);
    }
    return gNet.addGate(func, signals);
  }

  static void gnet1(eda::gate::model::GNet &gNet) {
    std::vector<Vertex> g(4);
    for (Vertex &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}));
    createLink(gNet, g, {5, 3});
  }

  static void gnet1Exteded(eda::gate::model::GNet &gNet) {
    std::vector<Vertex> g(4);
    for (Vertex &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}));
    g.push_back(createLink(gNet, g, {5, 3}));
    createLink(gNet, g, {5, 6}, eda::gate::model::GateSymbol::Value::OUT);
  }

  static void gnet2(eda::gate::model::GNet &gNet) {
    std::vector<Vertex> g(4);
    for (Vertex &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {3, 2}));
    createLink(gNet, g, {5, 4});
  }

  static void gnet3(eda::gate::model::GNet &gNet) {
    std::vector<Vertex> g(7);
    for (Vertex &el: g) {
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


    createLink(gNet, g, {11, 12, 14});
  }

  GNet *findConePrint(const std::filesystem::path &subCatalog, GNet *net,
                      const std::vector<GNet::V> &cuNodes, GNet::V start) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath = homePath / subCatalog;

    std::string filename1 = outputPath / "cone0.dot";
    std::string filename2 = outputPath / "cone.dot";

    Dot printer(net);
    printer.print(filename1);

    Cut cut;
    for (auto node: cuNodes) {
      cut.emplace(node);
    }

    ConeVisitor coneVisitor(cut, net);
    Walker walker(net, &coneVisitor, nullptr);
    walker.walk(start, false);

    GNet *subnet = coneVisitor.getGNet();
    // TODO: delete?
    std::cout << "gates size " << subnet->nGates() << std::endl;
    std::cout << "origin gates size " << net->nGates() << std::endl;

    printer = Dot(subnet);
    printer.print(filename2);

    return subnet;
  }

  void substitutePrint(const std::filesystem::path &subCatalog,
                       GNet *net, GNet *subNet,
                       const std::unordered_set<GNet::V> &cut) {
    const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
    const std::filesystem::path outputPath = homePath / subCatalog;

    std::string filename1 = outputPath / "gnet1.dot";
    std::string filename2 = outputPath / "gnet2.dot";
    std::string filename12 = outputPath / "gnet12.dot";

    Dot mainGnetDot(net);
    Dot subGnetDot(subNet);

    mainGnetDot.print(filename1);
    subGnetDot.print(filename2);

    substitute(6, cut, subNet, net);

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

// TODO: move std::vector<Vertex> g to each test to provide correct mapping with node id.
//===----------------------------------------------------------------------===//
// Tests
//===----------------------------------------------------------------------===//

  TEST(OptimizerTest, simulatorTestCone) {
    GNet net;
    gnet1(net);

    auto subnet = findConePrint("test/data/gate/optimizer/output/simulator",
                                &net, {2, 4}, 5);

    // simulatorPrint(subnet, {2, 4}, {5});
  }

  TEST(OptimizerTest, simulatorUse) {
    GNet net;

    gnet2(net);

    simulatorPrint(&net, {0, 1, 2, 3}, {6});
  }

//===----------------------------------------------------------------------===//-

  TEST(OptimizerTest, findCone) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet1(net);

    findConePrint("test/data/gate/optimizer/output/findCone1", &net, {2, 4}, 5);
  }

  TEST(OptimizerTest, findCone2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet net;
    gnet3(net);

    findConePrint("test/data/gate/optimizer/output/findCone3", &net, {0, 3, 7},
                  8);
  }

//===----------------------------------------------------------------------===//

  TEST(OptimizerTest, substitute) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }
    GNet mainGnet;
    GNet subGnet;

    gnet1(mainGnet);
    gnet2(subGnet);

    substitutePrint("test/data/gate/optimizer/output/substitute", &mainGnet,
                    &subGnet, mainGnet.getSources());
    EXPECT_EQ(mainGnet.nGates(), subGnet.nGates());
  }

  TEST(OptimizerTest, substitute2) {
    if (!getenv("UTOPIA_HOME")) {
      FAIL() << "UTOPIA_HOME is not set.";
    }

    GNet mainGnet;
    GNet subGnet;
    gnet1Exteded(mainGnet);
    gnet2(subGnet);

    substitutePrint("test/data/gate/optimizer/output/substitute2", &mainGnet,
                    &subGnet, mainGnet.getSources());
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
        Vertex failed;

        if (!isCut(v, c, failed)) {
          FAIL() << "Wrong cut for v " << v << "; failed " << failed << "\n";
        }
      }
    }

    delete parser.getGnet();
  }
}
