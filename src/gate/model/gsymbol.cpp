//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gsymbol.h"

#include <iostream>

namespace eda::gate::model {

std::ostream& operator <<(std::ostream &out, GateSymbol gate) {
  switch (gate) {
  case GateSymbol::ZERO:
    return out << "0";
  case GateSymbol::ONE:
    return out << "1";
  case GateSymbol::NOP:
    return out << "buf";
  case GateSymbol::NOT:
    return out << "not";
  case GateSymbol::AND:
    return out << "and";
  case GateSymbol::OR:
    return out << "or";
  case GateSymbol::XOR:
    return out << "xor";
  case GateSymbol::NAND:
    return out << "nand";
  case GateSymbol::NOR:
    return out << "nor";
  case GateSymbol::XNOR:
    return out << "xnor";
  case GateSymbol::LATCH:
    return out << "latch";
  case GateSymbol::DFF:
    return out << "dff";
  case GateSymbol::DFFrs:
    return out << "dff_rs";
  }

  return out;
}

} // namespace eda::gate::model
