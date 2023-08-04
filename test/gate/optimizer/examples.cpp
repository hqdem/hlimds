//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/examples.h"

namespace eda::gate::optimizer {

  GateId createLink(GNet &gNet, const std::vector<GateId> &g,
                    const std::vector<GateId> &input,
                    model::GateSymbol func) {
    std::vector<base::model::Signal<GNet::GateId>> signals;
    for (GateId gate: input) {
      signals.emplace_back(base::model::Event::ALWAYS, g[gate]);
    }
    return gNet.addGate(func, signals);
  }

  std::vector<GateId> gnet1(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}));
    g.push_back(createLink(gNet, g, {5, 3}));
    g.push_back(createLink(gNet, g, {6}, model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateId> gnet1Extended(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}));
    g.push_back(createLink(gNet, g, {5, 3}));
    g.push_back(createLink(gNet, g, {5, 6}));

    g.push_back(createLink(gNet, g, {static_cast<GateId>(g.size() - 1)},
                           model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateId> gnet1ChangedFunc(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}, model::GateSymbol::XOR));
    g.push_back(createLink(gNet, g, {5, 3}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {6}, model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateId> gnet2(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {3, 2}));
    g.push_back(createLink(gNet, g, {5, 4}));

    g.push_back(createLink(gNet, g, {static_cast<GateId>(g.size() - 1)},
                           model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateId> gnet2Extended(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {3, 2}));
    g.push_back(createLink(gNet, g, {4}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {5, 4}));

    g.push_back(createLink(gNet, g, {static_cast<GateId>(g.size() - 1)},
                           model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateId> gnet3(GNet &gNet) {
    std::vector<GateId> g(7);
    for (GateId &el: g) {
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

  std::vector<GateId> gnet3Cone(GNet &gNet) {
    std::vector<GateId> g(3);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {0, 2, 3}));

    g.push_back(createLink(gNet, g, {static_cast<GateId>(g.size() - 1)},
                           model::GateSymbol::OUT));
    return g;
  }

  std::vector<GateId> gnet4(GNet &gNet) {
    std::vector<GateId> g(3);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1, 2}));

    g.push_back(createLink(gNet, g, {static_cast<GateId>(g.size() - 1)},
                           model::GateSymbol::OUT));
    return g;
  }

  Visitor::MatchMap createPrimitiveMap(GNet *substNet, const Cut &cut) {
    std::unordered_map<GateId, GateId> map;

    auto it = cut.begin();
    for (GateId gateId: substNet->getSources()) {
      if (it == cut.end()) {
        break;
      }
      map[gateId] = *it;
      ++it;
    }

    return map;
  }

  /* in1   in2   in3   in4              */
  /* ┌─┐   ┌─┐   ┌─┐   ┌─┐              */
  /* └─┘─┐ └─┘─┐ └─┘─┐ └─┘─┐            */
  /* ┌─┐ |_┌─┐ |_┌─┐ |_┌─┐ |_┌─┐   ┌─┐  */
  /* └─┘───└─┘───└─┘───└─┘───└─┘───└─┘  */
  /* in0   and5  and6  and7  and8  out9 */
  std::vector<GateID> balanceAND(GNet &gNet) {
    std::vector<GateID> g(5);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {5, 2}));
    g.push_back(createLink(gNet, g, {6, 3}));
    g.push_back(createLink(gNet, g, {7, 4}));
    g.push_back(createLink(gNet, g, {8}, model::GateSymbol::OUT));
    return g;
  }

  /* in1   in2   in3   in4   in5              */
  /* ┌─┐   ┌─┐   ┌─┐   ┌─┐   ┌─┐              */
  /* └─┘─┐ └─┘─┐ └─┘─┐ └─┘─┐ └─┘─┐            */
  /* ┌─┐ |_┌─┐ |_┌─┐ |_┌─┐ |_┌─┐ |_┌─┐   ┌─┐  */
  /* └─┘───└─┘───└─┘───└─┘───└─┘───└─┘───└─┘  */
  /* in0   and6  and7  and8  and9 and10 out11 */
  std::vector<GateID> balanceANDTwice(GNet &gNet) {
    std::vector<GateID> g(6);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {6, 2}));
    g.push_back(createLink(gNet, g, {7, 3}));
    g.push_back(createLink(gNet, g, {8, 4}));
    g.push_back(createLink(gNet, g, {9, 5}));
    g.push_back(createLink(gNet, g, {10}, model::GateSymbol::OUT));
    return g;
  }

  /* Three has AND and NOT opertations.                     */
  /* Balacing net once in left subnet of upper AND gate,    */
  /* once in right subnet of upper AND gate, then balancing */
  /* once on upper gate.                                    */
  std::vector<GateID> balanceANDThrice(GNet &gNet) {
    std::vector<GateID> g(9);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {9, 2}));
    g.push_back(createLink(gNet, g, {10, 3}));
    g.push_back(createLink(gNet, g, {5, 6}));
    g.push_back(createLink(gNet, g, {12, 7}));
    g.push_back(createLink(gNet, g, {13, 8}));
    g.push_back(createLink(gNet, g, {14}, model::GateSymbol::NOT));
    g.push_back(createLink(gNet, g, {4, 15}));
    g.push_back(createLink(gNet, g, {16, 11}));
    g.push_back(createLink(gNet, g, {17}, model::GateSymbol::OUT));
    return g;
  }

  /* Three has AND, OR and NOT opertations.            */
  /* Same GNet as previous one, but operations are not */
  /* associative between each other.                   */
  std::vector<GateID> unbalancableANDOR(GNet &gNet) {
    std::vector<GateID> g(9);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}, model::GateSymbol::AND));
    g.push_back(createLink(gNet, g, {9, 2}, model::GateSymbol::AND));
    g.push_back(createLink(gNet, g, {10, 3}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {5, 6}, model::GateSymbol::AND));
    g.push_back(createLink(gNet, g, {12, 7}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {13, 8}, model::GateSymbol::AND));
    g.push_back(createLink(gNet, g, {14}, model::GateSymbol::NOT));
    g.push_back(createLink(gNet, g, {4, 15}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {16, 11}, model::GateSymbol::AND));
    g.push_back(createLink(gNet, g, {17}, model::GateSymbol::OUT));
    return g;
  }

  /* ┌─┐   ┌─┐  */
  /* └─┘───└─┘  */
  /* in0   out1 */
  std::vector<GateID> oneInOneOut(GNet &gNet) {
    std::vector<GateID> g(1);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0}, model::GateSymbol::OUT));
    return g;
  }

  /* in0   in2   in3              */
  /* ┌─┐   ┌─┐   ┌─┐              */
  /* └─┘─┐ └─┘─┐ └─┘─┐            */
  /* ┌─┐ |_┌─┐ |_┌─┐ |_┌─┐   ┌─┐  */
  /* └─┘───└─┘───└─┘───└─┘───└─┘  */
  /* in1   or4   or5   or6   out7 */
  std::vector<GateID> balanceOR(GNet &gNet) {
    std::vector<GateID> g(4);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {4, 2}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {5, 3}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {6}, model::GateSymbol::OUT));
    return g;
  }

  /* in0   in2   in3              */
  /* ┌─┐   ┌─┐   ┌─┐              */
  /* └─┘─┐ └─┘─┐ └─┘─┐            */
  /* ┌─┐ |_┌─┐ |_┌─┐ |_┌─┐   ┌─┐  */
  /* └─┘───└─┘───└─┘───└─┘───└─┘  */
  /* in1   xor4  xnor5 xor6  out7 */
  std::vector<GateID> balanceXORXNOR(GNet &gNet) {
    std::vector<GateID> g(4);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}, model::GateSymbol::XOR));
    g.push_back(createLink(gNet, g, {4, 2}, model::GateSymbol::XNOR));
    g.push_back(createLink(gNet, g, {5, 3}, model::GateSymbol::XOR));
    g.push_back(createLink(gNet, g, {6}, model::GateSymbol::OUT));
    return g;
  }

  /* in0   in2   in3              */
  /* ┌─┐   ┌─┐   ┌─┐              */
  /* └─┘─┐ └─┘─┐ └─┘─┐            */
  /* in1 | and4| and5| and6  out8 */
  /* ┌─┐ |_┌─┐ |_┌─┐ |_┌─┐   ┌─┐  */
  /* └─┘───└─┘───└─┘───└─┘───└─┘  */
  /*               |out7┌─┐       */
  /*               └────└─┘       */
  std::vector<GateID> balanceSeveralOut(GNet &gNet) {
    std::vector<GateID> g(4);
    for (GateID &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}));
    g.push_back(createLink(gNet, g, {5, 3}));
    g.push_back(createLink(gNet, g, {6}, model::GateSymbol::OUT));
    g.push_back(createLink(gNet, g, {5}, model::GateSymbol::OUT));
    return g;
  }

} // namespace eda::gate::optimizer
