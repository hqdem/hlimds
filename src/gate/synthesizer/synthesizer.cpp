//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/logger.h"
#include "gate/synthesizer/operation/addition.h"
#include "gate/synthesizer/operation/bitwise.h"
#include "gate/synthesizer/operation/comparison.h"
#include "gate/synthesizer/operation/division.h"
#include "gate/synthesizer/operation/multiplexer.h"
#include "gate/synthesizer/operation/multiplication.h"
#include "gate/synthesizer/operation/negation.h"
#include "gate/synthesizer/operation/reduction.h"
#include "gate/synthesizer/operation/shift.h"
#include "gate/synthesizer/synthesizer.h"

#include <cassert>
#include <iostream>

namespace eda::gate::synthesizer {

using namespace eda::gate::model;

static SubnetID synthImpl(const CellType &type) {
  const auto &attr = type.getAttr();

  switch (type.getSymbol()) {
  case BNOTs:  return synthBNotS(attr);
  case BNOTu:  return synthBNotU(attr);
  case BANDs:  return synthBAndS(attr);
  case BANDu:  return synthBAndU(attr);
  case BORs:   return synthBOrS(attr);
  case BORu:   return synthBOrU(attr);
  case BXORs:  return synthBXorS(attr);
  case BXORu:  return synthBXorU(attr);
  case BNANDs: return synthBNandS(attr);
  case BNANDu: return synthBNandU(attr);
  case BNORs:  return synthBNorS(attr);
  case BNORu:  return synthBNorU(attr);
  case BXNORs: return synthBXnorS(attr);
  case BXNORu: return synthBXnorU(attr);
  case RAND:   return synthRAnd(attr);
  case ROR:    return synthROr(attr);
  case RXOR:   return synthRXor(attr);
  case RNAND:  return synthRNand(attr);
  case RNOR:   return synthRNor(attr);
  case RXNOR:  return synthRXnor(attr);
  case MUX2:   return synthMux2(attr);
  case EQs:    return synthEqS(attr);
  case EQu:    return synthEqU(attr);
  case NEQs:   return synthNeqS(attr);
  case NEQu:   return synthNeqU(attr);
  case LTs:    return synthLtS(attr);
  case LTu:    return synthLtU(attr);
  case LTEs:   return synthLteS(attr);
  case LTEu:   return synthLteU(attr);
  case GTs:    return synthGtS(attr);
  case GTu:    return synthGtU(attr);
  case GTEs:   return synthGteS(attr);
  case GTEu:   return synthGteU(attr);
  case NEG:    return synthNeg(attr);
  case ADD:    return synthAdd(attr);
  case SUB:    return synthSub(attr);
  case MULs:   return synthMulS(attr);
  case MULu:   return synthMulU(attr);
  case DIVs:   return synthDivS(attr);
  case DIVu:   return synthDivU(attr);
  case REMs:   return synthRemS(attr);
  case REMu:   return synthRemU(attr);
  case MODs:   return synthModS(attr);
  case SHL:    return synthShl(attr);
  case SHRu:   return synthShrU(attr);
  case SHRs:   return synthShrS(attr);
  default:     return OBJ_NULL_ID;
  }
}

void synthSoftBlocks(const NetID netID) {
  auto &net = Net::get(netID);

  auto softBlocks = net.getSoftBlocks();
  for (const auto &cellID : softBlocks) {
    auto &cell = Cell::get(cellID);
    auto &type = const_cast<CellType&>(cell.getType());

    const auto subnetID = synthImpl(type);
    if (subnetID == OBJ_NULL_ID) {
       UTOPIA_WARN("Unsupported soft block type "
          << type.getName() << " (treated as a hard block)");
    } else {
      type.setSubnet(subnetID);
    }
  }
}

} // namespace eda::gate::synthesizer
