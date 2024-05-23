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

static size_t getLinkNum(const Simulator::Subnet &subnet) {
  const auto &entries = subnet.getEntries();

  size_t n = 0;
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto &cell = entries[i].cell;
    const auto items = (cell.isOut() ? 1u : cell.getOutNum());

    n += items;
    i += cell.more;
  }

  return n;
}

Simulator::Simulator(const Subnet &subnet):
    state(getLinkNum(subnet)),
    pos(subnet.getEntries().size()),
    subnet(subnet) {
  const auto &entries = subnet.getEntries();
  program.reserve(subnet.size());

  size_t p = 0;
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto &cell = entries[i].cell;

    if (!cell.isIn()) {
      const auto op = getFunction(cell, i);
      program.emplace_back(op, i, subnet.getLinks(i));
    }

    pos[i] = p;

    i += cell.more;
    p += (cell.isOut() ? 1 : cell.getOutNum());
  }
}

Simulator::Function Simulator::getFunction(const Cell &cell, size_t idx) const {
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
  default               : return getCell(idx, nIn, nOut);
  }

  return getZero(0);
}

} // namespace eda::gate::simulator
