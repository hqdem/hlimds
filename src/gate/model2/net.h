//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/cell.h"
#include "gate/model2/list.h"

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Net
//===----------------------------------------------------------------------===//

class Net final : public Object<Net, NetID> {
  friend class Storage<Net>;

public:
  List<CellID> getInputs() const { return List<CellID>(inputs); }
  List<CellID> getOutputs() const { return List<CellID>(outputs); }
  List<CellID> getCombCells() const { return List<CellID>(combCells); }
  List<CellID> getFlipFlops() const { return List<CellID>(flipFlops); }
  List<CellID> getSoftBlocks() const { return List<CellID>(softBlocks); }
  List<CellID> getHardBlocks() const { return List<CellID>(hardBlocks); }

  /// Returns the number of inputs.
  uint16_t getInNumber() const { return nInputs; }
  /// Returns the number of outputs.
  uint16_t getOutNumber() const { return nOutputs; }
  /// Returns the number of combinational gates/cells.
  uint32_t getCombNumber() const { return nCombCells; }
  /// Returns the number of flip-flops and latches.
  uint32_t getFlipNumber() const { return nFlipFlops; }
  /// Returns the number of hard blocks.
  uint16_t getHardNumber() const { return nHardBlocks; }
  /// Returns the number of soft blocks and subnets.
  uint16_t getSoftNumber() const { return nSoftBlocks; }

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
  NetID make();
 
private:
  List<CellID> inputs;
  List<CellID> outputs;
  List<CellID> combCells;
  List<CellID> flipFlops;
  List<CellID> hardBlocks;
  List<CellID> softBlocks;
};

} // namespace eda::gate::model
