//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include "kitty/kitty.hpp"

#include <cassert>

namespace eda::gate::model {

inline kitty::dynamic_truth_table getLinkTable(
    const Subnet::Link &link,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  const auto &table = tables[link.idx];
  return link.inv ? ~table : table;
}

inline kitty::dynamic_truth_table evaluateIn(const Subnet &subnet, size_t i) {
  auto table = kitty::create<kitty::dynamic_truth_table>(subnet.getInNum());
  kitty::create_nth_var(table, i);
  return table;
}

inline kitty::dynamic_truth_table evaluateOut(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  return getLinkTable(cell.link[0], tables); 
}

inline kitty::dynamic_truth_table evaluateZero(const Subnet &subnet) {
  auto table = kitty::create<kitty::dynamic_truth_table>(subnet.getInNum());
  kitty::clear(table);
  return table;
}

inline kitty::dynamic_truth_table evaluateOne(const Subnet &subnet) {
  return ~evaluateZero(subnet);
}

inline kitty::dynamic_truth_table evaluateBuf(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  return getLinkTable(cell.link[0], tables); 
}

inline kitty::dynamic_truth_table evaluateNot(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  return ~getLinkTable(cell.link[0], tables); 
}

inline kitty::dynamic_truth_table evaluateAnd(
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

inline kitty::dynamic_truth_table evaluateOr(
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

inline kitty::dynamic_truth_table evaluateXor(
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

inline kitty::dynamic_truth_table evaluateNand(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t i,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  return ~evaluateAnd(subnet, cell, i, tables);
}

inline kitty::dynamic_truth_table evaluateNor(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t i,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  return ~evaluateOr(subnet, cell, i, tables);
}

inline kitty::dynamic_truth_table evaluateXnor(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t i,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  return ~evaluateXor(subnet, cell, i, tables);
}

inline kitty::dynamic_truth_table evaluateMaj(
    const Subnet &subnet,
    const Subnet::Cell &cell,
    const size_t i,
    const std::vector<kitty::dynamic_truth_table> &tables) {
  auto table = evaluateZero(subnet);

  kitty::dynamic_truth_table args[cell.arity];
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

inline kitty::dynamic_truth_table evaluate(const Subnet &subnet) {
  assert(subnet.getInNum() > 0);
  assert(subnet.getOutNum() == 1);

  const auto &entries = subnet.getEntries();

  std::vector<kitty::dynamic_truth_table> tables;
  tables.reserve(entries.size());

  for (size_t i = 0; i < entries.size(); ++i) {
    const auto &cell = entries[i].cell;
    assert(!cell.isNull());

    kitty::dynamic_truth_table table;

    if (cell.isIn())        { table = evaluateIn  (subnet,       i        ); }
    else if (cell.isOut())  { table = evaluateOut (subnet, cell,    tables); }
    else if (cell.isZero()) { table = evaluateZero(subnet                 ); }
    else if (cell.isOne())  { table = evaluateOne (subnet                 ); }
    else if (cell.isBuf())  { table = evaluateBuf (subnet, cell,    tables); }
    else if (cell.isNot())  { table = evaluateNot (subnet, cell,    tables); }
    else if (cell.isAnd())  { table = evaluateAnd (subnet, cell, i, tables); }
    else if (cell.isOr())   { table = evaluateOr  (subnet, cell, i, tables); }
    else if (cell.isXor())  { table = evaluateXor (subnet, cell, i, tables); }
    else if (cell.isNand()) { table = evaluateNand(subnet, cell, i, tables); }
    else if (cell.isNor())  { table = evaluateNor (subnet, cell, i, tables); }
    else if (cell.isXnor()) { table = evaluateXnor(subnet, cell, i, tables); }
    else if (cell.isMaj())  { table = evaluateMaj (subnet, cell, i, tables); }
    else                    { assert(false && "Unsupported operation");      }

    tables.push_back(table);
    
    if (cell.more) {
      i += cell.more;
      for (size_t j = 0; j < cell.more; ++j) {
        tables.push_back(kitty::create<kitty::dynamic_truth_table>(0));
      }
    }
  }

  return tables.back();
}

} // namespace eda::gate::model
