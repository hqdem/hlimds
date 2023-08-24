//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/net.h"

#include <cassert>

namespace eda::gate::model {

void NetBuilder::addCell(CellID cellID) {
  auto *cell = access<Cell>(cellID);
  auto *type = access<CellType>(cell->getTypeID());

  // TODO: Implement structural hashing.

  if (type->getSymbol() == IN) {
    inputs.push_back(cellID);
  } else if (type->getSymbol() == OUT) {
    outputs.push_back(cellID);
  } else if (type->getSymbol() == NET || type->getSymbol() == SOFT) {
    softBlocks.push_back(cellID);
  } else if (type->getSymbol() == HARD) {
    hardBlocks.push_back(cellID);
  } else if (type->isCombinational()) {
    combCells.push_back(cellID);
  } else {
    flipFlops.push_back(cellID);
  }
}

NetID NetBuilder::makeNet() {
  uint16_t nInputs = inputs.size();
  assert(nInputs == inputs.size());

  uint16_t nOutputs = outputs.size();
  assert(nOutputs == outputs.size());

  uint32_t nCombCells = combCells.size();
  assert(nCombCells == combCells.size());

  uint32_t nFlipFlops = flipFlops.size();
  assert(nFlipFlops == flipFlops.size());
  
  uint16_t nHardBlocks = hardBlocks.size();
  assert(nHardBlocks == hardBlocks.size());

  uint16_t nSoftBlocks = softBlocks.size();
  assert(nSoftBlocks == softBlocks.size());

  return allocate<Net>(
    inputs.getID(),
    outputs.getID(),
    combCells.getID(),
    flipFlops.getID(),
    hardBlocks.getID(),
    softBlocks.getID(),
    nInputs,
    nOutputs,
    nCombCells,
    nFlipFlops,
    nHardBlocks,
    nSoftBlocks);
}

} // namespace eda::gate::model
