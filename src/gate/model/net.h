//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/logger.h"
#include "gate/model/cell.h"
#include "gate/model/list.h"
#include "gate/model/object.h"
#include "gate/model/storage.h"

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Net
//===----------------------------------------------------------------------===//

class Net final : public Object<Net, NetID> {
  friend class Storage<Net>;

public:
  Net &operator =(const Net &r) = delete;
  Net(const Net &r) = delete;

  List<CellID> getInputs() const { return List<CellID>(inputs); }
  List<CellID> getOutputs() const { return List<CellID>(outputs); }
  List<CellID> getCombCells() const { return List<CellID>(combCells); }
  List<CellID> getFlipFlops() const { return List<CellID>(flipFlops); }
  List<CellID> getSoftBlocks() const { return List<CellID>(softBlocks); }
  List<CellID> getHardBlocks() const { return List<CellID>(hardBlocks); }

  /// Returns the number of inputs.
  uint16_t getInNum() const { return nInputs; }
  /// Returns the number of outputs.
  uint16_t getOutNum() const { return nOutputs; }
  /// Returns the number of combinational gates/cells.
  uint32_t getCombNum() const { return nCombCells; }
  /// Returns the number of flip-flops and latches.
  uint32_t getFlipNum() const { return nFlipFlops; }
  /// Returns the number of hard blocks.
  uint16_t getHardNum() const { return nHardBlocks; }
  /// Returns the number of soft blocks and subnets.
  uint16_t getSoftNum() const { return nSoftBlocks; }

  /// Returns the overall number of cells.
  size_t getCellNum() const {
    size_t nIn   = getInNum();
    size_t nOut  = getOutNum();
    size_t nComb = getCombNum();
    size_t nFlip = getFlipNum();
    size_t nHard = getHardNum();
    size_t nSoft = getSoftNum();

    return nIn + nOut + nComb + nFlip + nHard + nSoft;
  }

private:
  Net(ListID inputs,
      ListID outputs,
      ListID combCells,
      ListID flipFlops,
      ListID hardBlocks,
      ListID softBlocks,
      uint16_t nInputs,
      uint16_t nOutputs,
      uint32_t nCombCells,
      uint32_t nFlipFlops,
      uint16_t nHardBlocks,
      uint16_t nSoftBlocks):
      inputs(inputs),
      outputs(outputs),
      combCells(combCells),
      flipFlops(flipFlops),
      hardBlocks(hardBlocks),
      softBlocks(softBlocks),
      nInputs(nInputs),
      nOutputs(nOutputs),
      nCombCells(nCombCells),
      nFlipFlops(nFlipFlops),
      nHardBlocks(nHardBlocks),
      nSoftBlocks(nSoftBlocks) {}

  /// Primary inputs.
  ListID inputs;
  /// Primary outputs.
  ListID outputs;

  /// Combinational gates/cells.
  ListID combCells;
  /// Triggers (flip-flops and latches).
  ListID flipFlops;

  /// Technology-dependent blocks w/ unknown structure and functionality.
  ListID hardBlocks;
  /// Blocks w/ known structure (subnets).
  ListID softBlocks;

  uint16_t nInputs;
  uint16_t nOutputs;
  uint32_t nCombCells;
  uint32_t nFlipFlops;
  uint16_t nHardBlocks;
  uint16_t nSoftBlocks;
};

static_assert(sizeof(Net) == NetID::Size);

//===----------------------------------------------------------------------===//
// Net Builder
//===----------------------------------------------------------------------===//

class NetBuilder final {
public:
  NetBuilder() {}
  void addCell(CellID cellID);
  void connect(CellID cellID, uint16_t port, LinkEnd source);
  NetID make();
 
private:
  void incRefCount(LinkEnd link) const;

  List<CellID> inputs;
  List<CellID> outputs;
  List<CellID> combCells;
  List<CellID> flipFlops;
  List<CellID> hardBlocks;
  List<CellID> softBlocks;
};

//===----------------------------------------------------------------------===//
// Net Validator
//===----------------------------------------------------------------------===//

bool validateNet(const Net &net, diag::Logger &logger);

inline bool validateNet(const NetID netID, diag::Logger &logger) {
  return validateNet(Net::get(netID), logger);
}

//===----------------------------------------------------------------------===//
// Net Printer
//===----------------------------------------------------------------------===//

std::ostream &operator <<(std::ostream &out, const Net &net);
 
} // namespace eda::gate::model
