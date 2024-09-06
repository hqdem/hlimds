//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"

namespace eda::gate::model {

static inline kitty::dynamic_truth_table getLinkTable(
    const Subnet::Link &link,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  const auto &table = tables[link.idx];
  return link.inv ? ~table : table;
}

static inline kitty::dynamic_truth_table evaluateIn(
    const Subnet &subnet, size_t i) {
  auto table = kitty::create<kitty::dynamic_truth_table>(subnet.getInNum());
  kitty::create_nth_var(table, i);
  return table;
}

static inline kitty::dynamic_truth_table evaluateOut(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  return getLinkTable(cell.link[0], tables); 
}

static inline kitty::dynamic_truth_table evaluateZero(
    const Subnet &subnet) {
  auto table = kitty::create<kitty::dynamic_truth_table>(subnet.getInNum());
  kitty::clear(table);
  return table;
}

static inline kitty::dynamic_truth_table evaluateOne(
    const Subnet &subnet) {
  return ~evaluateZero(subnet);
}

static inline kitty::dynamic_truth_table evaluateBuf(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  return getLinkTable(cell.link[0], tables); 
}

static inline kitty::dynamic_truth_table evaluateAnd(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t i,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  auto table = getLinkTable(cell.link[0], tables);
  for (size_t j = 1; j < cell.arity; ++j) {
    table &= getLinkTable(subnet.getLink(i, j), tables);
  }
  return table;
}

static inline kitty::dynamic_truth_table evaluateOr(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t i,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  auto table = getLinkTable(cell.link[0], tables);
  for (size_t j = 1; j < cell.arity; ++j) {
    table |= getLinkTable(subnet.getLink(i, j), tables);
  }
  return table;
}

static inline kitty::dynamic_truth_table evaluateXor(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t i,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  auto table = getLinkTable(cell.link[0], tables);
  for (size_t j = 1; j < cell.arity; ++j) {
    table ^= getLinkTable(subnet.getLink(i, j), tables);
  }
  return table;
}

static inline kitty::dynamic_truth_table evaluateMaj(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t i,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  auto table = evaluateZero(subnet);

  std::vector<kitty::dynamic_truth_table> args(cell.arity);
  for (size_t j = 0; j < cell.arity; ++j) {
    args[j] = getLinkTable(subnet.getLink(i, j), tables);
  }

  const auto threshold = cell.arity >> 1;
  for (size_t k = 0; k < table.num_bits(); ++k) {
    auto count = 0;
    for (size_t j = 0; j < cell.arity; ++j) {
      if (get_bit(args[j], k)) count++;
    }
    if (count > threshold) {
      set_bit(table, k);
    }
  }

  return table;
}

static inline kitty::dynamic_truth_table evaluateDummy() {
  return kitty::create<kitty::dynamic_truth_table>(0);
}

static std::vector<kitty::dynamic_truth_table> evaluate(
    const Subnet &subnet, std::vector<kitty::dynamic_truth_table> &tables) {
  std::vector<kitty::dynamic_truth_table> result;
  result.reserve(subnet.getOutNum());

  const auto &entries = subnet.getEntries();
  for (size_t i = 0; i < entries.size(); ++i) {
    if (i < tables.size()) continue;

    const auto &cell = entries[i].cell;
    assert(!cell.isNull());

    kitty::dynamic_truth_table table;

         if (cell.isIn())   { table = evaluateIn  (subnet,       i        ); }
    else if (cell.isOut())  { table = evaluateOut (subnet, cell,    tables); }
    else if (cell.isZero()) { table = evaluateZero(subnet                 ); }
    else if (cell.isOne())  { table = evaluateOne (subnet                 ); }
    else if (cell.isBuf())  { table = evaluateBuf (subnet, cell,    tables); }
    else if (cell.isAnd())  { table = evaluateAnd (subnet, cell, i, tables); }
    else if (cell.isOr())   { table = evaluateOr  (subnet, cell, i, tables); }
    else if (cell.isXor())  { table = evaluateXor (subnet, cell, i, tables); }
    else if (cell.isMaj())  { table = evaluateMaj (subnet, cell, i, tables); }
    else {
      const auto &type = cell.getType();
      assert(type.isSubnet() && "Unspecified subnet");

      const auto &impl = type.getSubnet();
      assert(impl.getInNum() == cell.arity && impl.getOutNum() == 1);

      std::vector<kitty::dynamic_truth_table> subtables;
      subtables.reserve(impl.size());

      for (size_t j = 1; j < cell.arity; ++j) {
        subtables.push_back(getLinkTable(subnet.getLink(i, j), tables));
      }

      table = evaluate(impl, subtables)[0];
    }

    tables.push_back(table);
    if (cell.isOut()) {
      result.push_back(table);
    }
    i += cell.more;

    if (i < entries.size()) {
      for (size_t j = 0; j < cell.more; ++j) {
        tables.push_back(evaluateDummy());
      }
    }
  }
  return result;
}

std::vector<kitty::dynamic_truth_table> evaluate(const Subnet &subnet) {
  std::vector<kitty::dynamic_truth_table> tables;
  tables.reserve(subnet.size());

  return evaluate(subnet, tables);
}

kitty::dynamic_truth_table computeCare(const Subnet &subnet) {
  const size_t nSets = (1ull << subnet.getInNum());
  const auto tables = evaluate(subnet);

  kitty::dynamic_truth_table care(subnet.getOutNum());
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
