//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/examples.h"

namespace eda::gate::optimizer {

  GateID createLink(GNet &gNet, const std::vector<GateID> &g,
                    const std::vector<GateID> &input,
                    model::GateSymbol func) {
    std::vector<base::model::Signal<GNet::GateId>> signals;
    for (GateID gate: input) {
      signals.emplace_back(base::model::Event::ALWAYS, g[gate]);
    }
    return gNet.addGate(func, signals);
  }

  std::vector<GateID> gnet1(GNet &gNet) {
    std::vector<GateID> g(4);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}));
    g.push_back(createLink(gNet, g, {5, 3}));
    g.push_back(createLink(gNet, g, {6}, model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateID> gnet1Extended(GNet &gNet) {
    std::vector<GateID> g(4);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}));
    g.push_back(createLink(gNet, g, {5, 3}));
    g.push_back(createLink(gNet, g, {5, 6}));

    g.push_back(createLink(gNet, g, {static_cast<GateID>(g.size() - 1)},
                           model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateID> gnet2(GNet &gNet) {
    std::vector<GateID> g(4);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {3, 2}));
    g.push_back(createLink(gNet, g, {5, 4}));

    g.push_back(createLink(gNet, g, {static_cast<GateID>(g.size() - 1)},
                           model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateID> gnet2Extended(GNet &gNet) {
    std::vector<GateID> g(4);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {3, 2}));
    g.push_back(createLink(gNet, g, {4}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {5, 4}));

    g.push_back(createLink(gNet, g, {static_cast<GateID>(g.size() - 1)},
                           model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateID> gnet3(GNet &gNet) {
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

    g.push_back(createLink(gNet, g, {13},
                           model::GateSymbol::OUT));
    g.push_back(createLink(gNet, g, {15},
                           model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateID> gnet3Cone(GNet &gNet) {
    std::vector<GateID> g(3);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {0, 2, 3}));

    g.push_back(createLink(gNet, g, {static_cast<GateID>(g.size() - 1)},
                           model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateID> gnet4(GNet &gNet) {
    std::vector<GateID> g(3);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1, 2}));

    g.push_back(createLink(gNet, g, {static_cast<GateID>(g.size() - 1)},
                           model::GateSymbol::OUT));
    return g;
  }

  Visitor::MatchMap createPrimitiveMap(GNet *substNet, const Cut &cut) {
    std::unordered_map<GateID, GateID> map;

    auto it = cut.begin();
    for (GateID gateId: substNet->getSources()) {
      if (it == cut.end()) {
        break;
      }
      map[gateId] = *it;
      ++it;
    }

    return map;
  }

} // namespace eda::gate::optimizer
