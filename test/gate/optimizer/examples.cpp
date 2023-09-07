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
    for (GateId GateId: substNet->getSources()) {
      if (it == cut.end()) {
        break;
      }
      map[GateId] = *it;
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
  std::vector<GateId> balanceAND(GNet &gNet) {
    std::vector<GateId> g(5);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {5, 2}));
    g.push_back(createLink(gNet, g, {6, 3}));
    g.push_back(createLink(gNet, g, {7, 4}));
    g.push_back(createLink(gNet, g, {8}, model::GateSymbol::OUT));
    return g;
  }

  /* in2                    */
  /* ┌─┐                    */
  /* └─┘─┐ and4  in3        */
  /* ┌─┐ |_┌─┐   ┌─┐        */
  /* └─┘───└─┘─┐ └─┘─┐      */
  /* in1   ┌─┐ |_┌─┐ |_┌─┐  */
  /*       └─┘───└─┘───└─┘  */
  /*       in0   and5  and6 */
  std::vector<GateId> balanceAND2(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {1, 2}));
    g.push_back(createLink(gNet, g, {0, 4}));
    g.push_back(createLink(gNet, g, {5, 3}));
    g.push_back(createLink(gNet, g, {6}, model::GateSymbol::OUT));
    return g;
  }

  /* in1   in2   in3   in4   in5              */
  /* ┌─┐   ┌─┐   ┌─┐   ┌─┐   ┌─┐              */
  /* └─┘─┐ └─┘─┐ └─┘─┐ └─┘─┐ └─┘─┐            */
  /* ┌─┐ |_┌─┐ |_┌─┐ |_┌─┐ |_┌─┐ |_┌─┐   ┌─┐  */
  /* └─┘───└─┘───└─┘───└─┘───└─┘───└─┘───└─┘  */
  /* in0   and6  and7  and8  and9 and10 out11 */
  std::vector<GateId> balanceANDTwice(GNet &gNet) {
    std::vector<GateId> g(6);
    for (GateId &el: g) {
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

  /* Net has AND and NOT opertations.                     */
  /* Balacing net once in left subnet of upper AND gate,    */
  /* once in right subnet of upper AND gate, then balancing */
  /* once on upper gate.                                    */
  std::vector<GateId> balanceANDThrice(GNet &gNet) {
    std::vector<GateId> g(9);
    for (GateId &el: g) {
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
    g.push_back(createLink(gNet, g, {11, 16}));
    g.push_back(createLink(gNet, g, {17}, model::GateSymbol::OUT));
    return g;
  }

  /* Net has AND, OR and NOT opertations.            */
  /* Same GNet as previous one, but operations are not */
  /* associative between each other.                   */
  std::vector<GateId> unbalancableANDOR(GNet &gNet) {
    std::vector<GateId> g(9);
    for (GateId &el: g) {
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
  std::vector<GateId> oneInOneOut(GNet &gNet) {
    std::vector<GateId> g(1);
    for (GateId &el: g) {
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
  std::vector<GateId> balanceOR(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &el: g) {
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
  std::vector<GateId> balanceXORXNOR(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &el: g) {
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
  std::vector<GateId> balanceSeveralOut(GNet &gNet) {
    std::vector<GateId> g(4);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {4, 2}));
    g.push_back(createLink(gNet, g, {5, 3}));
    g.push_back(createLink(gNet, g, {6}, model::GateSymbol::OUT));
    g.push_back(createLink(gNet, g, {5}, model::GateSymbol::OUT));
    return g;
  }

  /* Net has AND operations. And gates with arity 3. */
  std::vector<GateId> balanceArity3(GNet &gNet) {
    std::vector<GateId> g(6);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {1, 2}));
    g.push_back(createLink(gNet, g, {6, 3, 4}));
    g.push_back(createLink(gNet, g, {0, 7, 5}));
    g.push_back(createLink(gNet, g, {8}, model::GateSymbol::OUT));
    return g;
  }

  /* Net has AND and OR operations. And gates with arity 4. */
  std::vector<GateId> balanceArity4(GNet &gNet) {
    std::vector<GateId> g(8);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {2, 3}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {8, 4, 5, 6}));
    g.push_back(createLink(gNet, g, {0, 1, 9, 7}));
    g.push_back(createLink(gNet, g, {10}, model::GateSymbol::OUT));
    return g;
  }

  /* Net has AND and OR operations. And gates with arity 4. */
  /* It is impossible to reduce depth if operations */
  /* are not commutative. */
  std::vector<GateId> balanceArity4_2(GNet &gNet) {
    std::vector<GateId> g(10);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {2, 3}, model::GateSymbol::OR));
    g.push_back(createLink(gNet, g, {10, 4, 5, 6}));
    g.push_back(createLink(gNet, g, {8, 9}));
    g.push_back(createLink(gNet, g, {7, 12}));
    g.push_back(createLink(gNet, g, {0, 1, 11, 13}));
    g.push_back(createLink(gNet, g, {14}, model::GateSymbol::OUT));
    return g;
  }

  /* Net has AND operations. And gates with arity 3 and 4.     */
  /* For not commutative operations test checks possibility to */
  /* move operations left and right.                           */
  std::vector<GateId> balanceArity4LR(GNet &gNet) {
    std::vector<GateId> g(10);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {3, 4}));
    g.push_back(createLink(gNet, g, {5, 6}));
    g.push_back(createLink(gNet, g, {1, 2, 10}));
    g.push_back(createLink(gNet, g, {11, 7, 8}));;
    g.push_back(createLink(gNet, g, {0, 12, 13, 9}));
    g.push_back(createLink(gNet, g, {14}, model::GateSymbol::OUT));
    return g;
  }

  /*    ┌─┐                       */
  /* in0└─┘─┐                     */
  /*    ┌─┐ └┌─┐and5              */
  /* in1└─┘──└─┘──┐               */
  /*       _______├─┐maj6         */
  /*      |  ┌─┐┌─└─┘┐            */
  /*    ┌─┤  └─┘┘in3 | maj7       */
  /* in2└─┘────────┐ |_┌─┐   out8 */
  /*    ┌─┐        └───| |   ┌─┐  */
  /* in4└─┘────────────└─┘───└─┘  */
  /*                              */
  std::vector<GateId> balanceMajLeft(GNet &gNet) {
    std::vector<GateId> g(5);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 1}));
    g.push_back(createLink(gNet, g, {5, 2, 3}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {6, 2, 4}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {7}, model::GateSymbol::OUT));
    return g;
  }

  /* Same GNet as "balanceMajLeft", but lower maj function  */
  /* is third input of upper one. And deeper input of lower */
  /* maj is also third.                                     */
  std::vector<GateId> balanceMajRight(GNet &gNet) {
    std::vector<GateId> g(5);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {3, 4}));
    g.push_back(createLink(gNet, g, {1, 2, 5}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {0, 2, 6}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {7}, model::GateSymbol::OUT));
    return g;
  }

  /*                ┌─┐in0          */
  /*                └─┴──┐          */
  /*             in1┌─┐  |          */
  /*       in2┌─┐   └─┴┐ |maj8 out9 */
  /*          └─┴────┐ └┬┴┐   ┌─┐   */
  /*       in3┌─┬──┐ | ┌┴─┘───└─┘   */
  /* in4┌─┐   └─┘  └┬┴┐|            */
  /*    └─┘───┌─┬───└─┘┘maj7        */
  /*    ┌─┐───└─┘and6               */
  /* in5└─┘                         */
  std::vector<GateId> balanceMajUnbalancable(GNet &gNet) {
    std::vector<GateId> g(6);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {4, 5}));
    g.push_back(createLink(gNet, g, {2, 3, 6}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {0, 1, 7}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {8}, model::GateSymbol::OUT));
    return g;
  }

  /* First and third inputs of uppder maj gate are maj */
  /* gates. Third input has greater depth. Test checks */
  /* choise between two lower maj gates.               */
  std::vector<GateId> balanceMaj2Variants(GNet &gNet) {
    std::vector<GateId> g(6);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {0, 2, 1}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {5}, model::GateSymbol::NOT));
    g.push_back(createLink(gNet, g, {4, 7}));
    g.push_back(createLink(gNet, g, {3, 2, 8}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {6, 2, 9}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {10}, model::GateSymbol::OUT));
    return g;
  }

  /*                     ┌─┐in0           */
  /*                     └─┴──┐           */
  /*         ┌──────────────┐ |           */
  /* in3     |  in1┌─┐      | |maj9 out10 */
  /*   ┌─────┘     └─┴────┐ └┬┴┐   ┌─┐    */
  /* ┌─┼────────────────┐ | ┌┴─┘───└─┘    */
  /* └─┤  in2┌─┐        └┬┴┐|             */
  /*   |     └─┘───┌─┬───└─┘┘maj7         */
  /*   └───────────┤─┘maj6                */
  /*               |                      */
  /* in4┌─┐──┬─┐───┘                      */
  /*    └─┘  └─┤and6                      */
  /*    ┌─┐────┘                          */
  /* in5└─┘                               */
  std::vector<GateId> balanceMajTwice(GNet &gNet) {
    std::vector<GateId> g(6);
    for (GateId &el: g) {
      el = gNet.newGate();
    }
    g.push_back(createLink(gNet, g, {4, 5}, model::GateSymbol::AND));
    g.push_back(createLink(gNet, g, {2, 3, 6}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {1, 3, 7}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {0, 3, 8}, model::GateSymbol::MAJ));
    g.push_back(createLink(gNet, g, {9}, model::GateSymbol::OUT));
    return g;
  }

} // namespace eda::gate::optimizer
