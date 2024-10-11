//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/celltype.h"
#include "gate/simulator/simulator.h"
#include "util/assert.h"

namespace eda::gate::simulator {

static size_t getStateSize(const Simulator::SubnetBuilderPtr &builder) {
  size_t nBits = 0;
  for (auto it = builder->begin(); it != builder->end(); it.nextCell()) {
    const auto &cell = builder->getCell(*it);
    nBits += (cell.isOut() ? 1u : cell.getOutNum());
  }
  return nBits;
}

Simulator::Simulator(const SubnetBuilderPtr &builder):
    state(getStateSize(builder)),
    pos(builder->getCellNum()),
    subnet(builder) {
  program.reserve(builder->getCellNum());

  size_t i = 0, p = 0;
  for (auto it = builder->begin(); it != builder->end(); it.nextCell()) {
    const auto entryID = *it;
    const auto &cell = builder->getCell(entryID);

    if (!cell.isIn()) {
      const auto op = getFunction(cell, entryID);
      program.emplace_back(op, entryID, builder->getLinks(entryID));
    }

    // Save the mapping between entries and indices.
    //const_cast<SubnetBuilder&>(builder).setDataVal<size_t>(entryID, i);
    builder->setDataVal<size_t>(entryID, i);

    pos[i] = p;

    i += 1;
    p += (cell.isOut() ? 1 : cell.getOutNum());
  }
}

Simulator::Function Simulator::getFunction(
    const Cell &cell, EntryID entryID) const {
  using CellSymbol = eda::gate::model::CellSymbol;

  const auto func = cell.getSymbol();
  const auto nIn  = cell.getInNum();
  const auto nOut = cell.getOutNum();

  switch (func) {
  case CellSymbol::OUT  : return getBuf(nIn);
  case CellSymbol::ZERO : return getZero(nIn);
  case CellSymbol::ONE  : return getOne(nIn);
  case CellSymbol::BUF  : return getBuf(nIn);
  case CellSymbol::NOT  : return getNot(nIn);
  case CellSymbol::AND  : return getAnd(nIn);
  case CellSymbol::OR   : return getOr(nIn);
  case CellSymbol::XOR  : return getXor(nIn);
  case CellSymbol::NAND : return getNand(nIn);
  case CellSymbol::NOR  : return getNor(nIn);
  case CellSymbol::XNOR : return getXnor(nIn);
  case CellSymbol::MAJ  : return getMaj(nIn);
  default               : return getCell(entryID, nIn, nOut);
  }

  return getZero(0);
}

} // namespace eda::gate::simulator
