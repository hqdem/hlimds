//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022-2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/celltype.h"
#include "gate/simulator2/simulator.h"
#include "util/assert.h"

namespace eda::gate::simulator2 {

Simulator::Simulator(const Subnet &subnet):
    state(subnet.size()), nIn(subnet.getInNum()) {
  const auto entries = subnet.getEntries();
  program.reserve(subnet.size());

  for (size_t i = 0; i < entries.size(); ++i) {
    const auto cell = entries[i].cell;

    if (!cell.isIn()) {
      program.emplace_back(getFunction(cell), i, subnet.getLinks(i));
      i += cell.more;
    }
  }
}

Simulator::Function Simulator::getFunction(const Cell &cell) const {
  using CellSymbol = eda::gate::model::CellSymbol;

  const auto f = cell.getSymbol();
  const auto k = cell.arity;

  switch (f) {
  case CellSymbol::OUT  : return getBuf(k);
  case CellSymbol::ZERO : return getZero(k);
  case CellSymbol::ONE  : return getOne(k);
  case CellSymbol::BUF  : return getBuf(k);
  case CellSymbol::NOT  : return getNot(k);
  case CellSymbol::AND  : return getAnd(k);
  case CellSymbol::OR   : return getOr(k);
  case CellSymbol::XOR  : return getXor(k);
  case CellSymbol::NAND : return getNand(k);
  case CellSymbol::NOR  : return getNor(k);
  case CellSymbol::XNOR : return getXnor(k);
  case CellSymbol::MAJ  : return getMaj(k);
  default: uassert(false, "Unsupported cell: " << f << std::endl);
  }

  return getZero(0);
}

} // namespace eda::gate::simulator2
