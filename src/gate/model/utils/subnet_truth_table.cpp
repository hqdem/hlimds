//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"

namespace eda::gate::model {

using TT = kitty::dynamic_truth_table;
using TTs = std::vector</* cell */ std::vector</* output */ TT>>;

static inline TT getLinkTable(const Subnet::Link &link, const TTs &tables) {
  const auto &table = tables[link.idx][link.out];
  return link.inv ? ~table : table;
}

static inline TT evaluateIn(size_t i, size_t origNumVars) {
  auto table = kitty::create<TT>(origNumVars);
  kitty::create_nth_var(table, i);
  return table;
}

static inline TT evaluateOut(const Subnet &subnet,
    const Subnet::Cell &cell, const TTs &tables) {
  return getLinkTable(cell.link[0], tables);
}

static inline TT evaluateZero(size_t origNumVars) {
  auto table = kitty::create<TT>(origNumVars);
  kitty::clear(table);
  return table;
}

static inline TT evaluateOne(size_t origNumVars) {
  return ~evaluateZero(origNumVars);
}

static inline TT evaluateBuf(const Subnet &subnet,
    const Subnet::Cell &cell, const TTs &tables) {
  return getLinkTable(cell.link[0], tables);
}

static inline TT evaluateAnd(const Subnet &subnet,
    const Subnet::Cell &cell, const size_t i, const TTs &tables) {
  auto table = getLinkTable(cell.link[0], tables);
  for (size_t j = 1; j < cell.getInNum(); ++j) {
    table &= getLinkTable(subnet.getLink(i, j), tables);
  }
  return table;
}

static inline TT evaluateOr(const Subnet &subnet,
    const Subnet::Cell &cell, const size_t i, const TTs &tables) {
  auto table = getLinkTable(cell.link[0], tables);
  for (size_t j = 1; j < cell.getInNum(); ++j) {
    table |= getLinkTable(subnet.getLink(i, j), tables);
  }
  return table;
}

static inline TT evaluateXor(const Subnet &subnet,
    const Subnet::Cell &cell, const size_t i, const TTs &tables) {
  auto table = getLinkTable(cell.link[0], tables);
  for (size_t j = 1; j < cell.getInNum(); ++j) {
    table ^= getLinkTable(subnet.getLink(i, j), tables);
  }
  return table;
}

static inline TT evaluateMaj(const Subnet &subnet,
    const Subnet::Cell &cell, const size_t i,
    const TTs &tables, size_t origNumVars) {
  auto table = evaluateZero(origNumVars);

  std::vector<TT> args(cell.getInNum());
  for (size_t j = 0; j < cell.getInNum(); ++j) {
    args[j] = getLinkTable(subnet.getLink(i, j), tables);
  }

  const auto threshold = cell.getInNum() >> 1;
  for (size_t k = 0; k < table.num_bits(); ++k) {
    auto count = 0;
    for (size_t j = 0; j < cell.getInNum(); ++j) {
      if (get_bit(args[j], k)) count++;
    }
    if (count > threshold) {
      set_bit(table, k);
    }
  }

  return table;
}

static std::vector<TT> evaluate(const Subnet &subnet, TTs &tables,
                                size_t origNumVars) {
  std::vector<TT> result;
  result.reserve(subnet.getOutNum());

  const auto &entries = subnet.getEntries();
  for (size_t i = 0; i < entries.size(); ++i) {
    if (i < tables.size()) continue;

    const auto &cell = entries[i].cell;
    const auto &type = cell.getType();

    if (type.isSubnet()) {
      const auto &impl = type.getSubnet();
      assert(impl.getInNum() == cell.getInNum());
      assert(impl.getOutNum() == cell.getOutNum());

      TTs subtables;
      subtables.reserve(impl.size());

      for (size_t j = 0; j < cell.getInNum(); ++j) {
        subtables.push_back({getLinkTable(subnet.getLink(i, j), tables)});
      }

      tables.push_back(evaluate(impl, subtables, origNumVars));
    } else {
      TT table;

           if (cell.isIn())   { table = evaluateIn  (i, origNumVars); }
      else if (cell.isOut())  { table = evaluateOut (subnet, cell, tables); }
      else if (cell.isZero()) { table = evaluateZero(origNumVars); }
      else if (cell.isOne())  { table = evaluateOne (origNumVars); }
      else if (cell.isBuf())  { table = evaluateBuf (subnet, cell,    tables); }
      else if (cell.isAnd())  { table = evaluateAnd (subnet, cell, i, tables); }
      else if (cell.isOr())   { table = evaluateOr  (subnet, cell, i, tables); }
      else if (cell.isXor())  { table = evaluateXor (subnet, cell, i, tables); }
      else if (cell.isMaj())  { table = evaluateMaj (subnet, cell, i, tables,
                                                     origNumVars); }
      else                    { assert(false && "Unknown subnet cell type"); }

      tables.push_back({table});

      if (cell.isOut()) {
        result.push_back(table);
      }
    }

    i += cell.more;

    if (i < entries.size()) {
      for (size_t j = 0; j < cell.more; ++j) {
        tables.push_back({});
      }
    }
  } // for cells

  return result;
}

std::vector<TT> evaluate(const Subnet &subnet) {
  TTs tables;
  tables.reserve(subnet.size());

  return evaluate(subnet, tables, subnet.getInNum());
}

TT computeCare(const Subnet &subnet) {
  const size_t nSets = (1ull << subnet.getInNum());
  const auto tables = evaluate(subnet);

  TT care(subnet.getOutNum());
  for (size_t i = 0; i < nSets; i++) {
    uint64_t careIndex = 0;
    for (size_t j = 0; j < tables.size(); ++j) {
      careIndex |= kitty::get_bit(tables[j], i) << j;
    }
    kitty::set_bit(care, careIndex);
  }
  return care;
}

} // namespace eda::gate::model
