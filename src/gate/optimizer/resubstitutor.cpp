//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/resubstitutor.h"

namespace eda::gate::optimizer {

using Fragment      = SubnetIteratorBase::SubnetFragment;
using Link          = model::Subnet::Link;
using LinkList      = model::Subnet::LinkList;
using Subnet        = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using Symbol        = model::CellSymbol;
using TruthTable    = kitty::dynamic_truth_table;
using TruthTables   = std::unordered_map<size_t, TruthTable>;

struct Inversions {
  bool invIn1;
  bool invIn2;
  bool invOut;
};

static void buildFromDivisor(const SubnetBuilder &builder,
                             SubnetBuilder &rhs,
                             size_t divisor,
                             std::unordered_map<size_t, size_t> &oldToNew) {

  if (oldToNew.find(divisor) != oldToNew.end()) {
    return;
  }

  auto links = builder.getLinks(divisor);
  auto symbol = builder.getCell(divisor).getSymbol();
  for (size_t i = 0; i < links.size(); ++i) {
    if (oldToNew.find(links[i].idx) == oldToNew.end()) {
      buildFromDivisor(builder, rhs, links[i].idx, oldToNew);
    }
    size_t idNew = oldToNew.at(links[i].idx);
    links[i].idx = idNew;
  }
  oldToNew[divisor] = rhs.addCell(symbol, links).idx;
}

static bool makeZeroResubstitution(const SubnetBuilder &builder,
                                   SafePasser &iter,
                                   const Subnet &cone,
                                   size_t divisor,
                                   std::unordered_map<size_t, size_t> &map,
                                   bool inv) {

  SubnetBuilder rhs;

  std::unordered_map<size_t, size_t> oldToNew;
  for (size_t i = 0; i < map.size() - 1; ++i) {
    oldToNew[map.at(i)] = rhs.addInput().idx;
  }

  buildFromDivisor(builder, rhs, divisor, oldToNew);

  const size_t outNew = rhs.addOutput(Link(oldToNew.at(divisor), inv)).idx;
  const size_t outOld = cone.size() - 1;
  if (outOld != outNew) {
    map[outNew] = map.at(outOld);
    map.erase(outOld);
  }

  const auto rhsID = rhs.make();

  iter.replace(rhsID, map);
  return true;
}

static bool checkZeroResubstitution(const TruthTable &target,
                                    const TruthTable &div,
                                    const TruthTable &care,
                                    bool &inv) {

  const size_t nBits = target.num_bits();
  const size_t nBitsDiv = div.num_bits();
  assert(nBits == nBitsDiv && "Unequal number of target and divisor bits!");

  size_t j = 1;
  for (size_t i = 0; i < nBits; ++i) {
    if (!kitty::get_bit(care, i)) {
      continue;
    }
    inv = kitty::get_bit(target, i) != kitty::get_bit(div, i);
    j = ++i;
    break;
  }

  for (; j < nBits; ++j) {
    if (!kitty::get_bit(care, j)) {
      continue;
    }
    if ((kitty::get_bit(target, j) != kitty::get_bit(div, j)) != inv) {
      return false;
    }
  }

  return true;
}

static bool makeZeroResubstitution(const SubnetBuilder &builder,
                                   SafePasser &iter,
                                   const TruthTable &target,
                                   const TruthTable &care,
                                   const Subnet &cone,
                                   const TruthTables &divsTT,
                                   std::unordered_map<size_t, size_t> &map) {

  bool inv = false;
  for (const auto &divisor : divsTT) {
    if (checkZeroResubstitution(target, divisor.second, care, inv)) {
      return makeZeroResubstitution(builder, iter, cone,
                                    divisor.first, map, inv);
    }
  }
  return false;
}

static bool makeOneResubstitution(SubnetBuilder &builder,
                                  SafePasser &iter,
                                  const Subnet &cone,
                                  size_t div1,
                                  size_t div2,
                                  std::unordered_map<size_t, size_t> &map,
                                  Inversions invs) {

  Link link1(div1, invs.invIn1);
  Link link2(div2, invs.invIn2);
  
  const auto idx = builder.addCell(Symbol::AND, link1, link2).idx;

  return makeZeroResubstitution(builder, iter, cone, idx, map, invs.invOut);
}

static bool checkOneResubstitution(const TruthTable &target,
                                   const TruthTable &div1,
                                   const TruthTable &div2,
                                   const TruthTable &care,
                                   size_t posOne,
                                   size_t posZero,
                                   Inversions &invs) {

  bool invIn1 = !kitty::get_bit(div1, posOne);
  bool invIn2 = !kitty::get_bit(div2, posOne);

  const TruthTable &div11 = invIn1 ? ~div1 : div1;
  const TruthTable &div21 = invIn2 ? ~div2 : div2;

  if (kitty::is_const0((target ^ (div11 & div21)) & care)) {
    invs.invIn1 = invIn1;
    invs.invIn2 = invIn2;
    invs.invOut = false;
    return true;
  }

  invIn1 = !kitty::get_bit(div1, posZero);
  invIn2 = !kitty::get_bit(div2, posZero);

  const TruthTable &div12 = invIn1 ? ~div1 : div1;
  const TruthTable &div22 = invIn2 ? ~div2 : div2;

  if (kitty::is_const0((~target ^ (div12 & div22)) & care)) {
    invs.invIn1 = invIn1;
    invs.invIn2 = invIn2;
    invs.invOut = true;
    return true;
  }

  return false;
}

static bool makeOneResubstitution(SubnetBuilder &builder,
                                  SafePasser &iter,
                                  const TruthTable &target,
                                  const TruthTable &care,
                                  const Subnet &cone,
                                  const TruthTables &divsTT,
                                  std::unordered_map<size_t, size_t> &map) {

  const size_t nBits = target.num_bits();

  size_t posOne = 0;
  for (size_t i = 0; i < nBits; ++i) {
    if (kitty::get_bit(care, i) && kitty::get_bit(target, i)) {
      posOne = i;
      break;
    }
  }
  size_t posZero = 0;
  for (size_t i = 0; i < nBits; ++i) {
    if (kitty::get_bit(care, i) && !kitty::get_bit(target, i)) {
      posZero = i;
      break;
    }
  }

  auto i = divsTT.begin();
  auto j = ++divsTT.begin();
  Inversions invs;
  for (; i != j; ++i) {
    for (; j != divsTT.end(); ++j) {
      const TruthTable &divTT1 = i->second;
      const TruthTable &divTT2 = j->second;
      const size_t div1 = i->first;
      const size_t div2 = j->first;
      if (checkOneResubstitution(target, divTT1, divTT2, care,
                                 posOne, posZero, invs)) {
        return makeOneResubstitution(builder, iter, cone,
                                     div1, div2, map, invs);
      }
    }
  }
  return false;
}

static bool makeConstResubstitution(SafePasser &iter,
                                    Symbol symbol,
                                    const Subnet &cone,
                                    std::unordered_map<size_t, size_t> &map) {

  SubnetBuilder rhs;
  rhs.addInputs(cone.getInNum());
  Link link = rhs.addCell(symbol);
  const size_t outNew = rhs.addOutput(link).idx;
  const auto rhsID = rhs.make();

  const size_t outOld = cone.size() - 1;
  if (outOld != outNew) {
    map[outNew] = map.at(outOld);
    map.erase(outOld);
  }

  iter.replace(rhsID, map);
  return true;
}

static bool checkConstResubstitution(const TruthTable &target,
                                     const TruthTable &care,
                                     bool &constant) {

  const size_t nBits = target.num_bits();
  assert(nBits == care.num_bits() && "Unequal number of care and target bits!");

  size_t j = 1;
  for (size_t i = 0; i < nBits; ++i) {
    if (!kitty::get_bit(care, i)) {
      continue;
    }
    constant = kitty::get_bit(target, i);
    j = ++i;
    break;
  }

  for (; j < nBits; ++j) {
    if (!kitty::get_bit(care, j)) {
      continue;
    }
    if (kitty::get_bit(target, j) != constant) {
        return false;
    }
  }
  return true;
}

static bool makeConstResubstitution(SafePasser &iter,
                                    const TruthTable &target,
                                    const TruthTable &care,
                                    const Subnet &cone,
                                    std::unordered_map<size_t, size_t> &map) {

  bool constant = true;
  if (checkConstResubstitution(target, care, constant)) {
    const Symbol symbol = constant ? Symbol::ONE : Symbol::ZERO;
    return makeConstResubstitution(iter, symbol, cone, map);
  }
  return false;
}

inline TruthTable getDivisorTable(const TruthTables &tables, Link link) {
  const auto &table = tables.at(link.idx);
  return link.inv ? ~table : table;
}

static TruthTable getZero(size_t inNum) {
  auto table = kitty::create<kitty::dynamic_truth_table>(inNum);
  kitty::clear(table);
  return table;
}

static TruthTable getAnd(const TruthTables &tables, const LinkList &links) {
  auto table = getDivisorTable(tables, links[0]);
  for (size_t i = 1; i < links.size(); ++i) {
    table &= getDivisorTable(tables, links[i]);
  }
  return table;
}

static TruthTable getOr(const TruthTables &tables, const LinkList &links) {
  auto table = getDivisorTable(tables, links[0]);
  for (size_t i = 1; i < links.size(); ++i) {
    table |= getDivisorTable(tables, links[i]);
  }
  return table;
}

static TruthTable getXor(const TruthTables &tables, const LinkList &links) {
  auto table = getDivisorTable(tables, links[0]);
  for (size_t i = 1; i < links.size(); ++i) {
    table ^= getDivisorTable(tables, links[i]);
  }
  return table;
}

static TruthTable getMaj(const TruthTables &tables, const LinkList &links) {
  auto table = getZero(tables.at(links[0].idx).num_vars());

  const auto nBits = tables.at(links[0].idx).num_bits();
  const auto arity = links.size();

  std::vector<TruthTable> args(arity);
  for (size_t i = 0; i < arity; ++i) {
    args[i] = getDivisorTable(tables, links[i]);
  }

  const auto half = arity >> 1;
  for (size_t i = 0; i < nBits; ++i) {
    size_t count = 0;
    for (size_t j = 0; j < arity; ++j) {
      if (kitty::get_bit(args[j], i)) count++;
    }
    if (count > half) {
      kitty::set_bit(table, i);
    }
  }

  return table;
}

static void fillTruthTables(TruthTables &tables,
                            size_t divisor,
                            const SubnetBuilder &builder,
                            size_t nLeaves) {

  if (tables.find(divisor) != tables.end()) {
    return;
  }

  const auto &cell = builder.getCell(divisor);
  if (cell.isZero()) {
    tables[divisor] = getZero(nLeaves);
    return;
  }
  if (cell.isOne()) {
    tables[divisor] = ~getZero(nLeaves);
    return;
  }

  const auto links = builder.getLinks(divisor);
  for (const auto &link : links) {
    fillTruthTables(tables, link.idx, builder, nLeaves);
  }

  if (cell.isBuf()) {
    tables[divisor] = getDivisorTable(tables, links[0]);
    return;
  }
  if (cell.isAnd()) {
    tables[divisor] = getAnd(tables, links);
    return;
  }
  if (cell.isOr()) {
    tables[divisor] = getOr(tables, links);
    return;
  }
  if (cell.isXor()) {
    tables[divisor] = getXor(tables, links);
    return;
  }
  if (cell.isMaj()) {
    tables[divisor] = getMaj(tables, links);
    return;
  }
  assert(false && "Unsupported cell symbol!");
}

static TruthTables getDivisorsTables(const SubnetBuilder &builder,
                                     const std::unordered_set<size_t> &divs,
                                     const std::vector<size_t> &leaves) {

  const size_t nLeaves = leaves.size();
  const size_t nDivs = divs.size();

  TruthTables tables;
  tables.reserve(nDivs);

  for (size_t i = 0; i < nLeaves; ++i) {
    auto table = kitty::create<TruthTable>(nLeaves);
    kitty::create_nth_var(table, i);
    tables[leaves[i]] = std::move(table);
  }

  for (const auto &div : divs) {
    fillTruthTables(tables, div, builder, nLeaves);
  }

  return tables;
}

static void collectDivisorsRecursively(std::unordered_set<size_t> &divisors,
                                       const SubnetBuilder &builder,
                                       size_t idx) {

  const auto &cell = builder.getCell(idx);
  if (cell.isOne() || cell.isZero() || (divisors.find(idx) != divisors.end())) {
    return;
  }

  divisors.insert(idx);

  const auto links = builder.getLinks(idx);
  for (const auto link : links) {
    collectDivisorsRecursively(divisors, builder, link.idx);
  }
}

static void collectDivisors(std::unordered_set<size_t> &divisors,
                            const SubnetBuilder &builder,
                            size_t rootID,
                            const std::unordered_map<size_t, size_t> &map) {

  for (auto it = map.begin(); it != map.end(); ++it) {
    size_t id = (*it).second;
    if (id == rootID) {
      continue;
    }
    collectDivisorsRecursively(divisors, builder, id);
  }
}

static std::unordered_set<size_t> getDivisors(const SubnetBuilder &builder,
                                              size_t rootID,
                                              const std::vector<size_t> &leaves,
                                              const Fragment &mffc) {

  std::unordered_set<size_t> divisors(leaves.begin(), leaves.end());

  collectDivisors(divisors, builder, rootID, mffc.entryMap);

  return divisors;
}

void Resubstitutor::transform(SubnetBuilder &builder) const {
  for (SafePasser iter = builder.begin();
      iter != builder.end() && !builder.getCell(*iter).isOut();
      ++iter) {
    
    const auto entryID = *iter;

    const auto &cell = builder.getCell(entryID);
    if (cell.isIn() || cell.isOne() || cell.isZero()) {
      continue;
    }

    std::unordered_map<size_t, size_t> map;
    const auto coneID = getReconvergenceCone(builder, entryID, cutSize, map);
    const auto &cone = Subnet::get(coneID);
  
    const auto target = model::evaluateSingleOut(cone);

    std::vector<size_t> leaves;
    leaves.reserve(cutSize);
    for (size_t i = 0; i < map.size() - 1; ++i) {
      leaves.push_back(map.at(i));
    }

    std::unordered_map<size_t, size_t> dummy;
    const auto careSubnetID = getReconvergenceWindow(builder, leaves,
                                                     careSize, dummy);
    const auto &careSubnet = Subnet::get(careSubnetID);
    const auto care = model::computeCare(careSubnet);

    if (makeConstResubstitution(iter, target, care, cone, map)) {
      continue;
    }

    const auto mffc   = getMffc(builder, entryID, leaves);
    const auto divs   = getDivisors(builder, entryID, leaves, mffc);
    const auto divsTT = getDivisorsTables(builder, divs, leaves);

    if (makeZeroResubstitution(builder, iter, target, care,
                               cone, divsTT, map)) {
      continue;
    }

    const auto &mffCone = Subnet::get(mffc.subnetID);
    const auto gain = mffCone.size() - mffCone.getInNum() - mffCone.getOutNum();
    if (gain < 2) {
      continue;
    }

    makeOneResubstitution(builder, iter, target, care, cone, divsTT, map);
  }
}

} // namespace eda::gate::optimizer
