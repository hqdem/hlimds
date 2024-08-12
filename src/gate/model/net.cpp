//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/net.h"
#include "gate/model/printer/printer.h"

#include <cassert>

namespace eda::gate::model {

void NetBuilder::incRefCount(LinkEnd link) const {
  auto &source = const_cast<Cell&>(link.getCell());
  assert(source.fanout != Cell::MaxFanout);
  source.fanout++;
}

void NetBuilder::addCell(CellID cellID) {
  assert(cellID != OBJ_NULL_ID);

  const auto &cell = Cell::get(cellID);
  const auto &type = cell.getType();

  if (type.isIn()) {
    inputs.push_back(cellID);
  } else if (type.isOut()) {
    outputs.push_back(cellID);
  } else if (type.isSoft()) {
    softBlocks.push_back(cellID);
  } else if (type.isHard()) {
    hardBlocks.push_back(cellID);
  } else if (type.isCombinational()) {
    combCells.push_back(cellID);
  } else {
    flipFlops.push_back(cellID);
  }

  const auto links = cell.getLinks();
  for (auto link : links) {
    if (!link.isValid()) {
      // Skip unconnected links (required to support cycles).
      continue;
    }
    incRefCount(link);
  }
}

void NetBuilder::connect(CellID cellID, uint16_t port, LinkEnd source) {
  assert(cellID != OBJ_NULL_ID);
  auto &cell = Cell::get(cellID);
  cell.setLink(port, source);
  incRefCount(source);
}

NetID NetBuilder::make() {
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

  return allocateObject<Net>(
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

//===----------------------------------------------------------------------===//
// Net Validator
//===----------------------------------------------------------------------===//

#define OBJECT(net) "Net"
#define PREFIX(net) OBJECT(net) << ": "

#define VALIDATE(logger, prop, msg)\
  if (!(prop)) {\
    DIAGNOSE_ERROR(logger, msg);\
    return false;\
  }

#define VALIDATE_NET(logger, net, prop, msg)\
  VALIDATE(logger, prop, PREFIX(net) << msg)

static bool validateCells(const List<CellID> &cells, diag::Logger &logger) {
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    if (!validateCell(*i, logger)) return false;
  }
  return true;
}

bool validateNet(const Net &net, diag::Logger &logger) {
  VALIDATE_NET(logger, net,
       net.getInNum() > 0,
      "No inputs");
  VALIDATE_NET(logger, net,
       net.getOutNum() > 0,
      "No outputs");
  VALIDATE_NET(logger, net,
      validateCells(net.getInputs(), logger),
      "[Incorrect net inputs]");
  VALIDATE_NET(logger, net,
      validateCells(net.getOutputs(), logger),
      "[Incorrect net outputs]");
  VALIDATE_NET(logger, net,
      validateCells(net.getCombCells(), logger),
      "[Incorrect net cells]");
  VALIDATE_NET(logger, net,
      validateCells(net.getFlipFlops(), logger),
      "[Incorrect net flip-flops/latches]");
  VALIDATE_NET(logger, net,
      validateCells(net.getSoftBlocks(), logger),
      "[Incorrect net soft macro-blocks]");
  VALIDATE_NET(logger, net,
      validateCells(net.getHardBlocks(), logger),
      "[Incorrect net hard macro-blocks]");
  return true;
}

//===----------------------------------------------------------------------===//
// Net Printer
//===----------------------------------------------------------------------===//

std::ostream &operator <<(std::ostream &out, const Net &net) {
  ModelPrinter::getDefaultPrinter().print(out, net);
  return out;
}

} // namespace eda::gate::model
