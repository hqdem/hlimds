//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer.h"
#include "gate/synthesizer/synthesizer_add.h"
#include "gate/synthesizer/synthesizer_cmp.h"
#include "gate/synthesizer/synthesizer_div.h"
#include "gate/synthesizer/synthesizer_mul.h"
#include "gate/synthesizer/synthesizer_mux.h"
#include "gate/synthesizer/synthesizer_neg.h"

#include <cassert>

namespace eda::gate::synthesizer {

using namespace eda::gate::model;

static SubnetID synthImpl(const CellType &type) {
  const auto &attr = type.getAttr();

  switch (type.getSymbol()) {
  case MUX2: return synthMux2(attr);
  case EQs:  return synthEqS(attr);
  case EQu:  return synthEqU(attr);
  case NEQs: return synthNeqS(attr);
  case NEQu: return synthNeqU(attr);
  case LTs:  return synthLtS(attr);
  case LTu:  return synthLtU(attr);
  case LTEs: return synthLteS(attr);
  case LTEu: return synthLteU(attr);
  case GTs:  return synthGtS(attr);
  case GTu:  return synthGtU(attr);
  case GTEs: return synthGteS(attr);
  case GTEu: return synthGteU(attr);
  case ADD:  return synthAdd(attr);
  case NEG:  return synthNeg(attr);
  case SUB:  return synthSub(attr);
  case MULs: return synthMulS(attr);
  case MULu: return synthMulU(attr);
  case DIVs: return synthDivS(attr);
  case DIVu: return synthDivU(attr);
  case REMs: return synthRemS(attr);
  case REMu: return synthRemU(attr);
  case MODs: return synthModS(attr);
  default: assert(false && "Unsupported operation");
  }

  return OBJ_NULL_ID;
}

void synthSoftBlocks(const NetID netID) {
  auto &net = Net::get(netID);

  auto softBlocks = net.getSoftBlocks();
  for (const auto &cellID : softBlocks) {
    auto &cell = Cell::get(cellID);
    auto &type = const_cast<CellType&>(cell.getType());

    const auto subnetID = synthImpl(type);
    type.setSubnet(subnetID);
  }
}

} // namespace eda::gate::synthesizer
