//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer.h"

#include <cassert>

namespace eda::gate::synthesizer {

using namespace eda::gate::model;

void synthSoftBlocks(const NetID netID) {
  auto &net = Net::get(netID);

  auto softBlocks = net.getSoftBlocks();
  for (const auto cellID : softBlocks) {
    auto &cell = Cell::get(cellID);
    auto &type = cell.getType();

    switch (type.getSymbol()) {
    case MUX2: break; // TODO:
    case EQs:  break; // TODO:
    case EQu:  break; // TODO:
    case NEQs: break; // TODO:
    case NEQu: break; // TODO:
    case LTs:  break; // TODO:
    case LTu:  break; // TODO:
    case LTEs: break; // TODO:
    case LTEu: break; // TODO:
    case GTs:  break; // TODO:
    case GTu:  break; // TODO:
    case GTEs: break; // TODO:
    case GTEu: break; // TODO:
    case ADD:  break; // TODO:
    case SUB:  break; // TODO:
    case MULs: break; // TODO:
    case MULu: break; // TODO:
    case DIVs: break; // TODO:
    case DIVu: break; // TODO:
    case REMs: break; // TODO:
    case REMu: break; // TODO:
    case MODs: break; // TODO:
    default: assert(false && "Unsupported operation");
    }

  }
}

} // namespace eda::gate::synthesizer
