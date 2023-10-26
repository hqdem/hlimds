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

    if (!cell.in) {
      program.emplace_back(getFunction(cell), i, subnet.getLinks(i));
      i += cell.more;
    }
  }
}

Simulator::Function Simulator::getFunction(const Cell &cell) const {
  using CellSymbol = eda::gate::model::CellSymbol;

  const auto f = cell.getSymbol();
  const auto n = cell.arity;

  switch (f) {
  case CellSymbol::OUT  : return getBuf(n);
  case CellSymbol::ZERO : return getZero(n);
  case CellSymbol::ONE  : return getOne(n);
  case CellSymbol::BUF  : return getBuf(n);
  case CellSymbol::NOT  : return getNot(n);
  case CellSymbol::AND  : return getAnd(n);
  case CellSymbol::OR   : return getOr(n);
  case CellSymbol::XOR  : return getXor(n);
  case CellSymbol::NAND : return getNand(n);
  case CellSymbol::NOR  : return getNor(n);
  case CellSymbol::XNOR : return getXnor(n);
  case CellSymbol::MAJ  : return getMaj(n);
  default: uassert(false, "Unsupported cell: " << f << std::endl);
  }

  return getZero(0);
}

} // namespace eda::gate::simulator2
