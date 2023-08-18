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

class Net final {
public:
  using ID = NetID;

  uint16_t getInNumber()   const { return nInputs;     }
  uint16_t getOutNumber()  const { return nOutputs;    }
  uint32_t getCombNumber() const { return nCombCells;  }
  uint32_t getFlipNumber() const { return nFlipFlops;  }
  uint16_t getHardNumber() const { return nHardBlocks; }
  uint16_t getSoftNumber() const { return nSoftBlocks; }

private:
  /// Primary inputs.
  ListID inputs;
  /// Primary outputs.
  ListID outputs;

  /// Combinational gates and library cells.
  ListID combCells;
  /// Triggers (flip-flops and latches).
  ListID flipFlops;

  /// Technology-dependent cells w/ unknown structure and functionality.
  ListID hardBlocks;
  /// Soft macro-blocks (subnets).
  ListID softBlocks;

  uint16_t nInputs;
  uint16_t nOutputs;
  uint32_t nCombCells;
  uint32_t nFlipFlops;
  uint16_t nHardBlocks;
  uint16_t nSoftBlocks;
};

static_assert(sizeof(Net) == NetID::Size);

} // namespace eda::gate::model
