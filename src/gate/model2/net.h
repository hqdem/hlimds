//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/list.h"
#include "gate/model2/storage.h"

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Net
//===----------------------------------------------------------------------===//

class Net final {
  friend class Storage<Net>;

public:
  using ID = NetID;

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
      ListID softBlocks):
      inputs(inputs),
      outputs(outputs),
      combCells(combCells),
      flipFlops(flipFlops),
      hardBlocks(hardBlocks),
      softBlocks(softBlocks),
      nInputs(0),
      nOutputs(0),
      nCombCells(0),
      nFlipFlops(0),
      nHardBlocks(0),
      nSoftBlocks(0) {}

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

// TODO:

} // namespace eda::gate::model
