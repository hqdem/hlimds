//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

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
  Net &operator =(const Net &) = delete;
  Net(const Net &) = delete;

  List<CellID> getInputs()     const { return List<CellID>(inputs);     }
  List<CellID> getOutputs()    const { return List<CellID>(outputs);    }
  List<CellID> getCombCells()  const { return List<CellID>(combCells);  }
  List<CellID> getFlipFlops()  const { return List<CellID>(flipFlops);  }
  List<CellID> getSoftBlocks() const { return List<CellID>(softBlocks); }
  List<CellID> getHardBlocks() const { return List<CellID>(hardBlocks); }

  /// Returns the number of inputs.
  uint32_t getInNum() const { return nInputs; }
  /// Returns the number of outputs.
  uint32_t getOutNum() const { return nOutputs; }
  /// Returns the overall number of cells.
  uint32_t getCellNum() const { return nCells; }

  /// Returns the number of combinational gates/cells.
  uint32_t getCombNum() const { return getCombCells().size(); }
  /// Returns the number of flip-flops and latches.
  uint32_t getFlipNum() const { return getFlipFlops().size(); }
  /// Returns the number of hard blocks.
  uint32_t getHardNum() const { return getHardBlocks().size(); }
  /// Returns the number of soft blocks and subnets.
  uint32_t getSoftNum() const { return getSoftBlocks().size(); }

private:
  Net(ListID inputs,
      ListID outputs,
      ListID combCells,
      ListID flipFlops,
      ListID hardBlocks,
      ListID softBlocks,
      uint32_t nInputs,
      uint32_t nOutputs,
      uint32_t nCells):
      inputs(inputs),
      outputs(outputs),
      combCells(combCells),
      flipFlops(flipFlops),
      hardBlocks(hardBlocks),
      softBlocks(softBlocks),
      nInputs(nInputs),
      nOutputs(nOutputs),
      nCells(nCells) {}

  /// Primary inputs.
  ListID inputs;
  /// Primary outputs.
  ListID outputs;

  /// Combinational gates/cells.
  ListID combCells;
  /// Sequential cells (flip-flops and latches).
  ListID flipFlops;

  /// Technology-dependent blocks w/ unknown structure and functionality.
  ListID hardBlocks;
  /// Blocks w/ known structure (subnets).
  ListID softBlocks;

  uint32_t nInputs;
  uint32_t nOutputs;
  uint32_t nCells;

  uint32_t padding__{0};
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
// Net Printer
//===----------------------------------------------------------------------===//

std::ostream &operator <<(std::ostream &out, const Net &net);
 
} // namespace eda::gate::model
