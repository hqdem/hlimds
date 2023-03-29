//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/optimizer_visitor.h"

namespace eda::gate::optimizer {

  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;
  using Vertex = GNet::V;
  using Simulator = eda::gate::simulator::Simulator;

  OptimizerVisitor::OptimizerVisitor(CutStorage *cutStorage,
                                     GNet *net) : cutStorage(
          cutStorage), net(net) {}

  VisitorFlags OptimizerVisitor::onNodeBegin(const GateID &node) {
    if (!net->contains(node)) {
      cutStorage->cuts.erase(node);
      return FINISH_THIS;
    }
    lastNode = node;
    return SUCCESS;
  }

  VisitorFlags OptimizerVisitor::onCut(const Visitor::Cut &cut) {
    for (auto node: cut) {
      if (!net->contains(node)) {
        auto &cuts = cutStorage->cuts[lastNode];
        auto toErase = cuts.find(cut);
        cuts.erase(toErase);
        return FINISH_THIS;
      }

      auto func = getTruthTable(lastNode, cut, net);
      // TODO: implement getSubnet method.
      auto *subNet =  getSubnet(func);
      if(fakeSubstitute(node, cut, subNet, net)) {
        substitute(node, cut, subNet, net);
        // TODO: we can return finish all here.
        // TODO: we can make list with nets and their profit.
      }
    }
    return FINISH_ALL;
  }

  VisitorFlags OptimizerVisitor::onNodeEnd(const GateID &) {
    return SUCCESS;
  }

  GNet *OptimizerVisitor::getSubnet(uint64_t func) {
    return nullptr;
  }

  // TODO: move to separate file.

  void substitute(GNet::V cutFor, CutStorage::Cut &cut, GNet *subsNet, GNet *net) {
    // nodes from subsNet -> nodes from gnet1
    std::unordered_map<GNet::V, GNet::V> nodes;
    std::queue<GNet::V> bfs;

    auto sourcesSubs = subsNet->getSources();
    auto sourceIt = sourcesSubs.begin();
    auto cutIt = cut.begin();

    // Making correspondence for sources of initial cut and given subNet.
    do {
      nodes[*sourceIt] = *cutIt;
      // TODO: delete
      std::cout << *sourceIt << " -> " << *cutIt << std::endl;
      for (const auto &link: Gate::get(*sourceIt)->links()) {
        bfs.push(link.target);
      }
    } while (++sourceIt != sourcesSubs.end() && ++cutIt != cut.end());

    while (!bfs.empty()) {
      if (nodes.find(bfs.front()) == nodes.end()) {
        const Gate *gate = Gate::get(bfs.front());
        bool finished = true;
        for (const auto &node: gate->inputs()) {
          if (nodes.find(node.node()) == nodes.end()) {
            bfs.push(node.node());
            finished = false;
          }
        }
        ///////
        if (finished) {
          std::vector<eda::base::model::Signal<GNet::GateId>> signals;

          for (const auto &input: gate->inputs()) {
            signals.emplace_back(input.event(), nodes[input.node()]);
          }

          if (gate->links().empty()) {
            nodes[gate->id()] = cutFor;
            assert(nodes.size() == subsNet->nGates());

            if (Gate::get(cutFor)->inputs() != signals) {
              // TODO: add links from cutFor node.
              // Deleting links.
              LinkCleanVisitor visitor(cutFor, net, signals);
              Walker walker(net, &visitor, nullptr);
              walker.walk(cutFor, false);
            }

          } else {
            nodes[gate->id()] = net->addGate(gate->func(), signals);
            // TODO: delete
            std::cout << gate->id() << " -> " << nodes[gate->id()] << std::endl;
            for (const auto &link: gate->links()) {
              bfs.push(link.target);
            }
          }

        }
        ///////
      }
      bfs.pop();
    }
  }

  bool OptimizerVisitor::fakeSubstitute(Vertex cutFor, const Cut &cut, GNet *subsNet, GNet *net) {
    return true;
  }

  uint64_t OptimizerVisitor::getTruthTable(Vertex cutFor, const Cut &cut, GNet *net) {
    GNet::LinkList out{Gate::Link(cutFor)};
    GNet::LinkList in;

    for (const auto &node: cut) {
      in.emplace_back(node);
    }

    Simulator simulator;

    auto compiled = simulator.compile(*net, in, out);

    uint64_t o;
    uint64_t res = 0;
    for (std::uint64_t i = 0; i < (1ull << cut.size()); i++) {
      compiled.simulate(o, i);
      // TODO: delete print.
      std::cout << std::hex << i << " -> " << o << std::endl;
      o <<= i;
      res |= o;
    }
    return res;
  }

} // namespace eda::gate::optimizer