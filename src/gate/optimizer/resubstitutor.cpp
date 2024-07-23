//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/resubstitutor.h"
#include "util/kitty_utils.h"

namespace eda::gate::optimizer {

//----------------------------------------------------------------------------//
// Main limitations
//----------------------------------------------------------------------------//

static constexpr size_t maxBranches = 8;
static constexpr size_t maxDivisors = 150;
static constexpr size_t maxDivisorsPairs = 500;
static_assert(maxBranches <= 32);

//----------------------------------------------------------------------------//
// Aliases
//----------------------------------------------------------------------------//

using Cell             = eda::gate::model::Subnet::Cell;
using IdxMap           = std::unordered_map<size_t, size_t>;
using InOutMapping     = eda::gate::model::InOutMapping;
using Link             = eda::gate::model::Subnet::Link;
using LinkList         = eda::gate::model::Subnet::LinkList;
using SubnetBuilder    = eda::gate::model::SubnetBuilder;
using SubnetView       = eda::gate::model::SubnetView;
using SubnetViewWalker = eda::gate::model::SubnetViewWalker;
using Symbol           = eda::gate::model::CellSymbol;
using TruthTable       = eda::utils::TruthTable;
using TruthTables      = std::vector<TruthTable>;

//----------------------------------------------------------------------------//
// Data structures
//----------------------------------------------------------------------------//

struct Divisor {
  Divisor() = delete;
  Divisor(uint64_t divisor) = delete;
  Divisor(const Divisor &div) = default;
  Divisor(size_t idx, bool inv): idx(idx), inv(inv) {}

  uint64_t idx : 63;
  uint64_t inv : 1;
};
static_assert(sizeof(Divisor) == 8);

struct DivisorsPair {
  DivisorsPair() = delete;
  DivisorsPair(Divisor d1, Divisor d2, bool f): first(d1), second(d2), inv(f) {}

  DivisorsPair operator~() const { return DivisorsPair(first, second, !inv); }

  Divisor first;
  Divisor second;
  bool inv;
};

/// @brief Divisors pairs truth tables storage.
class DivisorsTT {
public:
  void reserve(size_t nPairs) {
    negativeTTs.reserve(nPairs);
    positiveTTs.reserve(nPairs);
  }

  void addPositiveTT(TruthTable &&table) {
    positiveTTs.push_back(table);
  }
  void addPositiveTT(const TruthTable &table) {
    positiveTTs.push_back(table);
  }
  void addNegativeTT(TruthTable &&table) {
    negativeTTs.push_back(table);
  }
  void addNegativeTT(const TruthTable &table) {
    negativeTTs.push_back(table);
  }

  const TruthTable &getPositiveTT(size_t i) const {
    return positiveTTs[i];
  }
  const TruthTable &getNegativeTT(size_t i) const {
    return negativeTTs[i];
  }

private:
  TruthTables negativeTTs;
  TruthTables positiveTTs;
};

/// @brief Divisors storage.
class Divisors {
public:
  Divisors() = default;

  size_t nUnates() const {
    return negUnate.size() +  posUnate.size() + binate.size();
  }
  size_t nPairs() const {
    return pairNeg.size() + pairPos.size();
  }

  void reserveUnates(size_t nDivisors) {
    negUnate.reserve(nDivisors);
    posUnate.reserve(nDivisors);
    binate.reserve(nDivisors);
  }
  void reservePairs(size_t nPairs) {
    pairNeg.reserve(nPairs);
    pairPos.reserve(nPairs);
  }

  void addPositive(Divisor div) { posUnate.push_back(div); }
  void addNegative(Divisor div) { negUnate.push_back(div); }
  void addBinate  (Divisor div) { binate.push_back(div);   }

  void addPositive(DivisorsPair divPair) { pairPos.push_back(divPair); }
  void addNegative(DivisorsPair divPair) { pairNeg.push_back(divPair); }

  size_t getPositiveNum() const { return posUnate.size(); }
  size_t getNegativeNum() const { return negUnate.size(); }
  size_t getBinateNum()   const { return binate.size();   }

  size_t getPositivePairNum() const { return pairPos.size(); }
  size_t getNegativePairNum() const { return pairNeg.size(); }

  Divisor getPositive(size_t i) const { return posUnate[i]; }
  Divisor getNegative(size_t i) const { return negUnate[i]; }
  Divisor getBinate  (size_t i) const { return binate[i];   }

  DivisorsPair getPositivePair(size_t i) const { return pairPos[i]; }
  DivisorsPair getNegativePair(size_t i) const { return pairNeg[i]; }

private:
  std::vector<Divisor> negUnate;
  std::vector<Divisor> posUnate;
  std::vector<Divisor> binate;

  std::vector<DivisorsPair> pairNeg;
  std::vector<DivisorsPair> pairPos;
};

/// @brief Class is used when cut > 6 only (Non optimized case).
class CellTables {
public:
  CellTables() = default;

  const TruthTable &back() const { return tables.back(); }
  
  size_t size() const { return tables.size(); }

  void push(const TruthTable  &table) { tables.push_back(table); }
  void push(      TruthTable &&table) { tables.push_back(table); }

  void clear() {
    tables.clear();
    firstBranchID = -1;
    firstOuterID = -1;
    nBranches = 0;
    nOuters = 0;
    pivotID = -1;
  }

  void setPivotID(size_t idx) { pivotID = idx; }

  void reserve(size_t nCells) {
    tables.reserve(nCells);
  }

  void pushBranch(const TruthTable &table) {
    if (firstBranchID == (size_t)-1) {
      firstBranchID = tables.size();
    }
    nBranches++;
    tables.push_back(table);
  }
  void pushBranch(TruthTable &&table) {
    if (firstBranchID == (size_t)-1) {
      firstBranchID = tables.size();
    }
    nBranches++;
    tables.push_back(table);
  }

  void pushOuter(const TruthTable &table) {
    if (firstOuterID == (size_t)-1) {
      firstOuterID = tables.size();
    }
    nOuters++;
    tables.push_back(table);
  }
  void pushOuter(TruthTable &&table) {
    if (firstOuterID == (size_t)-1) {
      firstOuterID = tables.size();
    }
    nOuters++;
    tables.push_back(table);
  }

  void setBranchTT(size_t pos, const TruthTable &table) {
    assert(pos < nBranches);
    tables[firstBranchID + pos] = table;
  }
  void setBranchTT(size_t pos, TruthTable &&table) {
    assert(pos < nBranches);
    tables[firstBranchID + pos] = table;
  }

  void setOuterTT(size_t pos, const TruthTable &table) {
    assert(pos < nOuters);
    tables[firstOuterID + pos] = table;
  }
  void setOuterTT(size_t pos, TruthTable &&table) {
    assert(pos < nOuters);
    tables[firstOuterID + pos] = table;
  }

  void invertPivotTT() {
    assert(pivotID < tables.size());
    tables[pivotID] = ~tables[pivotID];
  }

private:
  TruthTables tables;
  size_t firstBranchID = -1;
  size_t firstOuterID = -1;
  size_t nBranches = 0;
  size_t nOuters = 0;
  size_t pivotID = -1;
};

//----------------------------------------------------------------------------//
// Convenient methods
//----------------------------------------------------------------------------//

static void buildFromDivisor(const SubnetBuilder &builder,
                             SubnetBuilder &rhs,
                             size_t idx,
                             IdxMap &oldToNew) {

  if (oldToNew.find(idx) != oldToNew.end()) {
    return;
  }

  auto links = builder.getLinks(idx);
  auto symbol = builder.getCell(idx).getSymbol();
  for (size_t i = 0; i < links.size(); ++i) {
    if (oldToNew.find(links[i].idx) == oldToNew.end()) {
      buildFromDivisor(builder, rhs, links[i].idx, oldToNew);
    }
    size_t idNew = oldToNew.at(links[i].idx);
    links[i].idx = idNew;
  }
  oldToNew[idx] = rhs.addCell(symbol, links).idx;
}

static TruthTable getTruthTable(const SubnetBuilder &builder,
                                size_t idx,
                                bool inv,
                                size_t arity) {

  TruthTable res;
  if (arity > 6) {
    res = inv ? ~utils::getTruthTable<utils::TTn>(builder, idx):
                 utils::getTruthTable<utils::TTn>(builder, idx);
  } else {
    auto tt = inv ? ~utils::getTruthTable<utils::TT6>(builder, idx):
                     utils::getTruthTable<utils::TT6>(builder, idx);
    res = utils::convertTruthTable<utils::TT6>(tt, arity);
  }

  return res;
}

static Link addCell(SubnetBuilder &builder,
                    Divisor div1,
                    Divisor div2,
                    Symbol symbol) {

  const Link link1(div1.idx, div1.inv);
  const Link link2(div2.idx, div2.inv);
  const Link res = builder.addCell(symbol, link1, link2);
  return res;
}

//----------------------------------------------------------------------------//
// Maximum fanout-free cone marking
//----------------------------------------------------------------------------//

static void markMffcRecursively(SubnetBuilder &builder, size_t idx) {
  if (builder.isMarked(idx)) {
    return;
  }

  builder.mark(idx);

  for (const auto &link : builder.getLinks(idx)) {
    markMffcRecursively(builder, link.idx);
  }
}

static size_t markMffc(SubnetBuilder &builder,
                       const SubnetView &view,
                       const IdxMap &mffc) {

  builder.startSession();

  for (size_t i = 0; i < mffc.size() - 1; ++i) {
    builder.mark(mffc.at(i));
  }
  markMffcRecursively(builder, view.getOut(0));

  const size_t res = builder.getSessionID();
  builder.endSession();

  return res;
}

//----------------------------------------------------------------------------//
// Divisors classifications
//----------------------------------------------------------------------------//

static std::pair<bool, Divisor> classifyDivisor(Divisor div,
                                                Divisors &divs,
                                                const TruthTable &table,
                                                const TruthTable &onset,
                                                const TruthTable &offset) {

  bool positive = false;
  bool negative = false;

  if (kitty::is_const0(table & offset)) {
    divs.addPositive(div);
    positive = true;
  }
  if (kitty::is_const0(~table & onset)) {
    divs.addNegative(div);
    negative = true;
  }
  if (positive && negative) {
    return std::make_pair(true, div);
  }
  if (!positive && !negative) {
    divs.addBinate(div);
  }

  return std::make_pair(false, div);
}

static std::pair<bool, Divisor> classifyDivisor(size_t idx,
                                                Divisors &divs,
                                                const TruthTable &table,
                                                const TruthTable &onset,
                                                const TruthTable &offset) {

  Divisor div1(idx, false);
  Divisor div2(idx, true);

  auto res = classifyDivisor(div1, divs, table, onset, offset);
  if (res.first) {
    return res;
  }
  res = classifyDivisor(div2, divs, ~table, onset, offset);

  return res;
}

static void classifyBinatePairs(SubnetBuilder &builder,
                                const SubnetView &view,
                                Divisors &divs,
                                DivisorsTT &divsTT,
                                const TruthTable &onset,
                                const TruthTable &offset) {

  const size_t arity = view.getInNum();
  for (size_t i = 0; i < divs.getBinateNum(); ++i) {
    for (size_t j = i + 1; j < divs.getBinateNum(); ++j) {
      const Divisor div1(divs.getBinate(i));
      const Divisor div2(divs.getBinate(j));

      if (div1.idx == div2.idx) {
        continue;
      }

      builder.mark(div1.idx);
      builder.mark(div2.idx);

      const auto tt1 = getTruthTable(builder, div1.idx, div1.inv, arity);
      const auto tt2 = getTruthTable(builder, div2.idx, div2.inv, arity);

      const auto tt = tt1 & tt2;

      const DivisorsPair divPair(div1, div2, false);

      if (kitty::is_const0(tt & offset)) {
        divs.addPositive(divPair);
        divsTT.addPositiveTT(std::move(tt));
      }
      else if (kitty::is_const0(~tt & offset)) {
        divs.addPositive(~divPair);
        divsTT.addPositiveTT(~tt);
      }
      else if (kitty::is_const0(~tt & onset)) {
        divs.addNegative(divPair);
        divsTT.addNegativeTT(std::move(tt));
      }
      else if (kitty::is_const0(tt & onset)) {
        divs.addNegative(~divPair);
        divsTT.addNegativeTT(~tt);
      }

      if (divs.nPairs() > maxDivisorsPairs) {
        return;
      }
    }
  }
}

//----------------------------------------------------------------------------//
// Divisors collecting from the both outer sides of the cone
//----------------------------------------------------------------------------//

static std::pair<bool, Divisor> getSideDivisors(SubnetBuilder &builder,
                                                const SubnetView &view,
                                                Divisors &divs,
                                                const TruthTable &onset,
                                                const TruthTable &offset,
                                                CellTables &cellTables,
                                                size_t mffcID,
                                                size_t idx) {

  if (builder.isMarked(idx) || (builder.getSessionID(idx) == mffcID)) {
    return std::make_pair(false, Divisor(0, 0));
  }
  const size_t maxDepth = builder.getDepth(view.getOut(0));
  if ((builder.getDepth(idx) > maxDepth) || (divs.nUnates() >= maxDivisors)) {
    return std::make_pair(false, Divisor(0, 0));
  }

  for (const auto &link : builder.getLinks(idx)) {
    if (!builder.isMarked(link.idx)) {
      return std::make_pair(false, Divisor(0, 0));
    }
  }

  if (builder.getSessionID(idx) != (builder.getSessionID() - 1)) {
    const size_t arity = view.getInNum();
    TruthTable divTT;
    builder.mark(idx);
    if (arity > 6) {
      divTT = utils::getTruthTable<utils::TTn>(builder, arity, idx, false, 0);
      cellTables.push(divTT);
      utils::setTruthTable<utils::TTn>(builder, idx, cellTables.back());
    } else {
      auto tt = utils::getTruthTable<utils::TT6>(builder, arity, idx, false, 0);
      utils::setTruthTable<utils::TT6>(builder, idx, tt);
      divTT = utils::convertTruthTable<utils::TT6>(tt, arity);
    }

    const auto res = classifyDivisor(idx, divs, divTT, onset, offset);
    if (res.first) {
      return res;
    }
  }
  builder.mark(idx);

  for (const auto &fanout : builder.getFanouts(idx)) {
    const auto res = getSideDivisors(
        builder, view, divs, onset, offset, cellTables, mffcID, fanout);

    if (res.first) {
      return res;
    }
  }

  return std::make_pair(false, Divisor(0, 0));
}

static std::pair<bool, Divisor> getSideDivisors(SubnetBuilder &builder,
                                                const SubnetView &view,
                                                Divisors &divs,
                                                const TruthTable &onset,
                                                const TruthTable &offset,
                                                CellTables &cellTables,
                                                size_t mffcID) {

  builder.startSession();

  for (const auto &in : view.getInputs()) {
    builder.mark(in);
  }

  for (const auto &in : view.getInputs()) {
    for (const auto &fanout : builder.getFanouts(in)) {
      const auto res = getSideDivisors(
          builder, view, divs, onset, offset, cellTables, mffcID, fanout);

      if (res.first) {
        builder.endSession();
        return res;
      }
    }
  }

  builder.endSession();
  return std::make_pair(false, Divisor(0, 0));
}

//----------------------------------------------------------------------------//
// Divisors collecting from the inputs of the mffc to the cut (part of the cone)
//----------------------------------------------------------------------------//

static std::pair<bool, Divisor> addInnerDivisor(const SubnetBuilder &builder,
                                                Divisors &divs,
                                                size_t idx,
                                                size_t arity,
                                                const TruthTable &onset,
                                                const TruthTable &offset) {

  TruthTable divisorTT;

  if (arity > 6) {
    divisorTT = utils::getTruthTable<utils::TTn>(builder, idx);
  } else {
    const auto tt = utils::getTruthTable<utils::TT6>(builder, idx);
    divisorTT = utils::convertTruthTable<utils::TT6>(tt, arity);
  }

  const auto res = classifyDivisor(idx, divs, divisorTT, onset, offset);

  return res;
}

static std::pair<bool, Divisor> getInnerDivisors(SubnetBuilder &builder,
                                                 Divisors &divs,
                                                 size_t idx,
                                                 size_t arity,
                                                 const TruthTable &onset,
                                                 const TruthTable &offset) {

  if (builder.isMarked(idx)) {
    return std::make_pair(false, Divisor(0, 0));
  }

  builder.mark(idx);

  auto res = addInnerDivisor(builder, divs, idx, arity, onset, offset);
  if (res.first) {
    return res;
  }

  for (const auto &link : builder.getLinks(idx)) {
    res = getInnerDivisors(builder, divs, link.idx, arity, onset, offset);
    if (res.first) {
      return res;
    }
  }

  return std::make_pair(false, Divisor(0, 0));
}

static std::pair<bool, Divisor> getInnerDivisors(SubnetBuilder &builder,
                                                 const SubnetView &view,
                                                 Divisors &divs,
                                                 const TruthTable &onset,
                                                 const TruthTable &offset,
                                                 const IdxMap &mffcMap) {

  builder.startSession();

  const auto &inputs = view.getInputs();
  const auto arity = inputs.size();

  // Add the cut.
  for (size_t i = 0; i < arity; ++i) {
    builder.mark(inputs[i]);
    const auto res = addInnerDivisor(
        builder, divs, inputs[i], arity, onset, offset);

    if (res.first) {
      builder.endSession();
      return res;
    }
  }
  // Get divisors from the inputs of the mffc to cut.
  for (size_t i = 0; i < mffcMap.size() - 1; ++i) {
    const auto res = getInnerDivisors(
        builder, divs, mffcMap.at(i), arity, onset, offset);
    
    if (res.first) {
      builder.endSession();
      return res;
    }
  }

  builder.endSession();
  return std::make_pair(false, Divisor(0, 0));
}

//----------------------------------------------------------------------------//
// Divisors collecting (inner + side)
//----------------------------------------------------------------------------//

static std::pair<bool, Divisor> getDivisors(SubnetBuilder &builder,
                                            const SubnetView &view,
                                            Divisors &divs,
                                            const TruthTable &onset,
                                            const TruthTable &offset,
                                            CellTables &cellTables,
                                            const IdxMap &mffc) {

  const size_t mffcID = markMffc(builder, view, mffc);
  auto res = getInnerDivisors(builder, view, divs, onset, offset, mffc);
  if (res.first) {
    return res;
  }

  res = getSideDivisors(builder, view, divs, onset, offset, cellTables, mffcID);

  return res;
}

//----------------------------------------------------------------------------//
// Resubstitution checking
//----------------------------------------------------------------------------//

static std::pair<bool, Divisor> checkNegativeUnate(SubnetBuilder &builder,
                                                   const Divisors &divs,
                                                   const TruthTable &offset,
                                                   size_t arity) {

  for (size_t i = 0; i < divs.getNegativeNum(); ++i) {
    for (size_t j = i + 1; j < divs.getNegativeNum(); ++j) {
      const Divisor div1 = divs.getNegative(i);
      const Divisor div2 = divs.getNegative(j);

      builder.mark(div1.idx);
      builder.mark(div2.idx);

      const TruthTable tt1 = getTruthTable(builder, div1.idx, div1.inv, arity);
      const TruthTable tt2 = getTruthTable(builder, div2.idx, div2.inv, arity);

      if (kitty::is_const0((tt1 & tt2) & offset)) {
        const size_t divID = addCell(builder, div1, div2, Symbol::AND).idx;
        return std::make_pair(true, Divisor(divID, false));
      }
    }
  }
  return std::make_pair(false, Divisor(0, 0));
}

static std::pair<bool, Divisor> checkPositiveUnate(SubnetBuilder &builder,
                                                   const Divisors &divs,
                                                   const TruthTable &onset,
                                                   size_t arity) {

  for (size_t i = 0; i < divs.getPositiveNum(); ++i) {
    for (size_t j = i + 1; j < divs.getPositiveNum(); ++j) {
      const Divisor div1 = divs.getPositive(i);
      const Divisor div2 = divs.getPositive(j);

      builder.mark(div1.idx);
      builder.mark(div2.idx);

      const TruthTable tt1 = getTruthTable(builder, div1.idx, div1.inv, arity);
      const TruthTable tt2 = getTruthTable(builder, div2.idx, div2.inv, arity);

      if (kitty::is_const0(~(tt1 | tt2) & onset)) {
        const size_t divID = addCell(builder, div1, div2, Symbol::OR).idx;
        return std::make_pair(true, Divisor(divID, false));
      }
    }
  }
  return std::make_pair(false, Divisor(0, 0));
}

static std::pair<bool, Divisor> checkNegativeUnatePair(SubnetBuilder &builder,
                                                       const Divisors &divs,
                                                       const DivisorsTT &divsTT,
                                                       const TruthTable &offset,
                                                       size_t arity) {

  for (size_t i = 0; i < divs.getNegativePairNum(); ++i) {
    for (size_t j = 0; j < divs.getNegativeNum(); ++j) {
      const DivisorsPair divPair = divs.getNegativePair(i);
      const Divisor div2 = divs.getNegative(j);
      const Divisor divF = divPair.first;
      const Divisor divS = divPair.second;

      builder.mark(div2.idx);

      const TruthTable &tt1 = divsTT.getNegativeTT(i);
      const TruthTable  tt2 = getTruthTable(builder, div2.idx, div2.inv, arity);

      if (kitty::is_const0((tt1 & tt2) & offset)) {
        Link link = addCell(builder, divF, divS, Symbol::AND);
        const Divisor div1(link.idx, link.inv ^ divPair.inv);

        const size_t divID = addCell(builder, div1, div2, Symbol::AND).idx;

        return std::make_pair(true, Divisor(divID, false));
      }
    }
  }
  return std::make_pair(false, Divisor(0, 0));
}

static std::pair<bool, Divisor> checkPositiveUnatePair(SubnetBuilder &builder,
                                                       const Divisors &divs,
                                                       const DivisorsTT &divsTT,
                                                       const TruthTable &onset,
                                                       size_t arity) {

  for (size_t i = 0; i < divs.getPositivePairNum(); ++i) {
    for (size_t j = 0; j < divs.getPositiveNum(); ++j) {
      const DivisorsPair divPair = divs.getPositivePair(i);
      const Divisor div2 = divs.getPositive(j);
      const Divisor divF = divPair.first;
      const Divisor divS = divPair.second;

      builder.mark(div2.idx);

      const TruthTable &tt1 = divsTT.getPositiveTT(i);
      const TruthTable  tt2 = getTruthTable(builder, div2.idx, div2.inv, arity);

      if (kitty::is_const0(~(tt1 | tt2) & onset)) {
        Link link = addCell(builder, divF, divS, Symbol::AND);
        const Divisor div1(link.idx, link.inv ^ divPair.inv);

        const size_t divID = addCell(builder, div1, div2, Symbol::OR).idx;

        return std::make_pair(true, Divisor(divID, false));
      }
    }
  }
  return std::make_pair(false, Divisor(0, 0));
}

static std::pair<bool, Divisor> checkNegativePairs(SubnetBuilder &builder,
                                                   const Divisors &divs,
                                                   const DivisorsTT &divsTT,
                                                   const TruthTable &offset,
                                                   size_t arity) {

  for (size_t i = 0; i < divs.getNegativePairNum(); ++i) {
    for (size_t j = i + 1; j < divs.getNegativePairNum(); ++j) {
      const DivisorsPair divPair1 = divs.getNegativePair(i);
      const DivisorsPair divPair2 = divs.getNegativePair(j);

      const Divisor div1F = divPair1.first;
      const Divisor div1S = divPair1.second;
      const Divisor div2F = divPair2.first;
      const Divisor div2S = divPair2.second;

      const TruthTable &tt1 = divsTT.getNegativeTT(i);
      const TruthTable &tt2 = divsTT.getNegativeTT(j);

      if (kitty::is_const0((tt1 & tt2) & offset)) {
        Link link1 = addCell(builder, div1F, div1S, Symbol::AND);
        Link link2 = addCell(builder, div2F, div2S, Symbol::AND);

        const Divisor div1(link1.idx, link1.inv ^ divPair1.inv);
        const Divisor div2(link2.idx, link2.inv ^ divPair2.inv);

        const size_t divID = addCell(builder, div1, div2, Symbol::AND).idx;

        return std::make_pair(true, Divisor(divID, false));
      }
    }
  }
  return std::make_pair(false, Divisor(0, 0));
}

static std::pair<bool, Divisor> checkPositivePairs(SubnetBuilder &builder,
                                                   const Divisors &divs,
                                                   const DivisorsTT &divsTT,
                                                   const TruthTable &onset,
                                                   size_t arity) {

  for (size_t i = 0; i < divs.getPositivePairNum(); ++i) {
    for (size_t j = i + 1; j < divs.getPositivePairNum(); ++j) {
      const DivisorsPair divPair1 = divs.getPositivePair(i);
      const DivisorsPair divPair2 = divs.getPositivePair(j);

      const Divisor div1F = divPair1.first;
      const Divisor div1S = divPair1.second;
      const Divisor div2F = divPair2.first;
      const Divisor div2S = divPair2.second;

      const TruthTable &tt1 = divsTT.getPositiveTT(i);
      const TruthTable &tt2 = divsTT.getPositiveTT(j);

      if (kitty::is_const0(~(tt1 | tt2) & onset)) {
        Link link1 = addCell(builder, div1F, div1S, Symbol::AND);
        Link link2 = addCell(builder, div2F, div2S, Symbol::AND);

        const Divisor div1(link1.idx, link1.inv ^ divPair1.inv);
        const Divisor div2(link2.idx, link2.inv ^ divPair2.inv);

        const size_t divID = addCell(builder, div1, div2, Symbol::OR).idx;

        return std::make_pair(true, Divisor(divID, false));
      }
    }
  }
  return std::make_pair(false, Divisor(0, 0));
}

//----------------------------------------------------------------------------//
// Resubstitutions (const, zero, one, two, three)
//----------------------------------------------------------------------------//

static bool makeResubstitution(const SubnetBuilder &builder,
                               SafePasser &iter,
                               const SubnetView &view,
                               Divisor div) {

  SubnetBuilder rhs;

  InOutMapping iomapping;
  iomapping.inputs = view.getInputs();
  iomapping.outputs = view.getOutputs();

  IdxMap oldToNew;
  for (size_t i = 0; i < iomapping.getInNum(); ++i) {
    oldToNew[iomapping.getIn(i)] = rhs.addInput().idx;
  }

  buildFromDivisor(builder, rhs, div.idx, oldToNew);

  Link link(oldToNew.at(div.idx), div.inv);
  rhs.addOutput(link);

  iter.replace(rhs, iomapping);

  return true;
}

static void makeConstResubstitution(SafePasser &iter,
                                    const SubnetView &view,
                                    bool constant) {

  SubnetBuilder rhs;
  rhs.addInputs(view.getInNum());

  const Symbol symbol = constant ? Symbol::ONE : Symbol::ZERO;

  Link link = rhs.addCell(symbol);
  rhs.addOutput(link);

  iter.replace(rhs, view.getInOutMapping());
}

static bool makeConstResubstitution(SafePasser &iter,
                                    const SubnetView &view,
                                    const TruthTable &onset,
                                    const TruthTable &offset) {

  bool zero = kitty::is_const0(onset);
  bool one = kitty::is_const0(offset);

  if (zero) {
    makeConstResubstitution(iter, view, false);
    return true;
  }
  if (one) {
    makeConstResubstitution(iter, view, true);
    return true;
  }
  return false;
}

static bool makeZeroResubstitution(SubnetBuilder &builder,
                                   SafePasser &iter,
                                   const SubnetView &view,
                                   Divisors &divs,
                                   const TruthTable &onset,
                                   const TruthTable &offset,
                                   CellTables &cellTables,
                                   const IdxMap &mffcMap) {

  const auto res = getDivisors(
      builder, view, divs, onset, offset, cellTables, mffcMap);

  if (res.first) {
    return makeResubstitution(builder, iter, view, res.second);
  }

  return false;
}

static bool makeOneResubstitution(SubnetBuilder &builder,
                                  SafePasser &iter,
                                  const SubnetView &view,
                                  const Divisors &divs,
                                  const TruthTable &onset,
                                  const TruthTable &offset) {

  builder.startSession();
  const size_t arity = view.getInNum();

  auto res = checkNegativeUnate(builder, divs, offset, arity);
  if (res.first) {
    builder.endSession();
    return makeResubstitution(builder, iter, view, res.second);
  }

  res = checkPositiveUnate(builder, divs, onset, arity);
  if (res.first) {
    builder.endSession();
    return makeResubstitution(builder, iter, view, res.second);
  }

  builder.endSession();
  return false;
}

static bool makeTwoResubstitution(SubnetBuilder &builder,
                                  SafePasser &iter,
                                  const SubnetView &view,
                                  Divisors &divs,
                                  DivisorsTT &divsTT,
                                  const TruthTable &onset,
                                  const TruthTable &offset) {

  builder.startSession();
  const size_t arity = view.getInNum();

  classifyBinatePairs(builder, view, divs, divsTT, onset, offset);

  auto res = checkNegativeUnatePair(builder, divs, divsTT, offset, arity);
  if (res.first) {
    builder.endSession();
    return makeResubstitution(builder, iter, view, res.second);
  }

  res = checkPositiveUnatePair(builder, divs, divsTT, onset, arity);
  if (res.first) {
    builder.endSession();
    return makeResubstitution(builder, iter, view, res.second);
  }

  builder.endSession();
  return false;
}

static bool makeThreeResubstitution(SubnetBuilder &builder,
                                    SafePasser &iter,
                                    const SubnetView &view,
                                    Divisors &divs,
                                    DivisorsTT &divsTT,
                                    const TruthTable &onset,
                                    const TruthTable &offset) {

  const size_t arity = view.getInNum();

  auto res = checkNegativePairs(builder, divs, divsTT, offset, arity);
  if (res.first) {
    return makeResubstitution(builder, iter, view, res.second);
  }

  res = checkPositivePairs(builder, divs, divsTT, onset, arity);
  if (res.first) {
    return makeResubstitution(builder, iter, view, res.second);
  }

  return false;
}

//----------------------------------------------------------------------------//
// Simulations
//----------------------------------------------------------------------------//

static void simulateCone(SubnetBuilder &builder,
                         const SubnetView &view,
                         CellTables &cellTables) {

  const size_t arity = view.getInNum();
  if (arity <= 6) {
    view.evaluateTruthTable();
  } else {
    const SubnetViewWalker walker(view);
    size_t nIn = 0;

    walker.run([&cellTables, &nIn, arity](SubnetBuilder &builder,
                                          const bool isIn,
                                          const bool isOut,
                                          const size_t i) -> bool {
      const auto tt = utils::getTruthTable<TruthTable>(
          builder, arity, i, isIn, nIn++);
      cellTables.push(std::move(tt));
      utils::setTruthTable<TruthTable>(builder, i, cellTables.back());
      return true; // Continue traversal.
    });
  }
}

static void invertPivotTT(SubnetBuilder &builder,
                          size_t pivot,
                          CellTables &cellTables,
                          size_t arity) {

  if (arity > 6) {
    cellTables.invertPivotTT();
  } else {
    const auto inverted = ~utils::getTruthTable<utils::TT6>(builder, pivot);
    utils::setTruthTable<utils::TT6>(builder, pivot, inverted);
  }
}

static TruthTables evaluateRoots(SubnetBuilder &builder,
                                 const SubnetView &view,
                                 size_t arity,
                                 CellTables &cellTables) {

  TruthTables result(view.getOutNum());

  const SubnetViewWalker walker(view);

  if (arity <= 6) {
    walker.run([arity](SubnetBuilder &builder,
                       const bool isIn,
                       const bool isOut,
                       const size_t i) -> bool {
                                
      if (isIn) {
        return true; // Continue traversal.
      }
      const auto tt = utils::getTruthTable<utils::TT6>(
          builder, arity, i, isIn, 0);
      utils::setTruthTable<utils::TT6>(builder, i, tt);
      return true; // Continue traversal.
    });

    for (size_t i = 0; i < view.getOutNum(); ++i) {
      const auto tt = utils::getTruthTable<utils::TT6>(builder, view.getOut(i));
      result[i] = utils::convertTruthTable<utils::TT6>(tt, arity);
    }
  } else {
    size_t nOuter = 0;
    walker.run([&nOuter, &cellTables, arity](SubnetBuilder &builder,
                                             const bool isIn,
                                             const bool isOut,
                                             const size_t i) -> bool {

      if (isIn) {
        return true; // Continue traversal.
      }
      auto tt = utils::getTruthTable<utils::TTn>(builder, arity, i, isIn, 0);
      cellTables.setOuterTT(nOuter++, std::move(tt));
      return true; // Continue traversal.
    });

    for (size_t i = 0; i < view.getOutNum(); ++i) {
      result[i] = utils::getTruthTable<utils::TTn>(builder, view.getOut(i));
    }
  }

  return result;
}

//----------------------------------------------------------------------------//
// Don't care evaluation (ODC)
//----------------------------------------------------------------------------//

static TruthTable computeCare(SubnetBuilder &builder,
                              uint64_t status,
                              const SubnetView &careView,
                              size_t pivot,
                              size_t arity,
                              const std::vector<size_t> &branches,
                              CellTables &cellTables) {

  auto care = utils::getZeroTruthTable<utils::TTn>(arity);

  // Init branches.
  for (size_t i = 0; i < branches.size(); ++i) {
    if (arity <= 6) {
      const auto constant = ((status >> i) & 1ull) ?
          utils::getOneTruthTable<utils::TT6>(arity):
          utils::getZeroTruthTable<utils::TT6>(arity);

      utils::setTruthTable<utils::TT6>(builder, branches[i], constant);
    } else {
      auto constant = ((status >> i) & 1ull) ?
          utils::getOneTruthTable<utils::TTn>(arity):
          utils::getZeroTruthTable<utils::TTn>(arity);

      cellTables.setBranchTT(i, std::move(constant));
    }
  }

  // Evaluate roots with standart pivot.
  const auto standart = evaluateRoots(builder, careView, arity, cellTables);

  // Evaluate roots with inverted pivot.
  invertPivotTT(builder, pivot, cellTables, arity);
  const auto inverted = evaluateRoots(builder, careView, arity, cellTables);
  invertPivotTT(builder, pivot, cellTables, arity);

  // Compare roots tables
  for (size_t i = 0; i < careView.getOutNum(); ++i) {
    care |= standart[i] ^ inverted[i];
  }

  return care;
}

static void markInnerRecursively(SubnetBuilder &builder, size_t idx) {
  if (builder.isMarked(idx)) {
    return;
  }

  builder.mark(idx);

  for (const auto &link : builder.getLinks(idx)) {
    markInnerRecursively(builder, link.idx);
  }
}

static size_t markInner(SubnetBuilder &builder, const SubnetView &view) {
  builder.startSession();

  for (const auto &input : view.getInputs()) {
    builder.mark(input);
  }
  for (const auto &pivot : view.getOutputs()) {
    markInnerRecursively(builder, pivot);
  }

  const size_t ret = builder.getSessionID();
  builder.endSession();
  return ret;
}

static size_t countSetBits(uint64_t n) {
  if (!n) {
    return 0;
  }
  return 1 + countSetBits(n & (n - 1));
}

static void prepareStatus(uint64_t &status, uint64_t iteration) {
  status &= 0xFFFFFFFF00000000;
  status |= iteration;
}

static void addInnerToInputs(SubnetBuilder &builder,
                             InOutMapping &iomapping,
                             size_t idx,
                             size_t innerID) {

  if (builder.isMarked(idx)) {
    return;
  }

  if (builder.getSessionID(idx) == innerID) {
    builder.mark(idx);
    iomapping.inputs.push_back(idx);
    return;
  }

  builder.mark(idx);

  for (const auto &link : builder.getLinks(idx)) {
    addInnerToInputs(builder, iomapping, link.idx, innerID);
  }
}

static SubnetView getCareView(SubnetBuilder &builder,
                              size_t pivot,
                              const std::vector<size_t> &roots,
                              const std::vector<size_t> &branches,
                              size_t k,
                              CellTables &cellTables,
                              size_t innerID) {

  builder.startSession();

  InOutMapping iomapping;
  if (k > 6) {
    for (size_t i = 0; i < branches.size(); ++i) {
      auto zero = utils::getZeroTruthTable<utils::TTn>(k);
      builder.mark(branches[i]);
      iomapping.inputs.push_back(branches[i]);
      cellTables.pushBranch(std::move(zero));
      utils::setTruthTable<utils::TTn>(builder, branches[i], cellTables.back());
    }
  } else {
    for (size_t i = 0; i < branches.size(); ++i) {
      const auto zero = utils::getZeroTruthTable<utils::TT6>(k);
      builder.mark(branches[i]);
      iomapping.inputs.push_back(branches[i]);
      utils::setTruthTable<utils::TT6>(builder, branches[i], zero);
    }
  }

  for (const auto &r : roots) {
    addInnerToInputs(builder, iomapping, r, innerID);
  }

  builder.endSession();

  iomapping.outputs = roots;
  SubnetView careView(builder, iomapping);
  return careView;
}

static void reserveOuters(const SubnetView &view,
                          CellTables &cellTables,
                          size_t arity) {

  const SubnetViewWalker walker(view);

  walker.run([&cellTables, arity](SubnetBuilder &parent,
                                  const bool isIn,
                                  const bool isOut,
                                  const size_t i) -> bool {

    if (isIn) {
      return true; // Continue traversal.
    }
    const auto zero = utils::getZeroTruthTable<utils::TTn>(arity);
    cellTables.pushOuter(std::move(zero));
    utils::setTruthTable<utils::TTn>(parent, i, cellTables.back());
    return true; // Continue traversal.
  });
}

static TruthTable computeCare(SubnetBuilder &builder,
                              const SubnetView &view,
                              const std::vector<size_t> &roots,
                              const std::vector<size_t> &branches,
                              uint64_t status,
                              CellTables &cellTables) {

  const size_t k = view.getInNum();
  const size_t pivot = view.getOut(0);

  // Mark inner nodes (from pivot to cut).
  const size_t innerID = markInner(builder, view);

  const auto careView = getCareView(builder, pivot, roots, branches,
                                    k, cellTables, innerID);

  if (k > 6) {
    reserveOuters(careView, cellTables, k);
  }

  auto care = utils::getZeroTruthTable<utils::TTn>(k);

  const size_t nSetBits = countSetBits(status >> 32);
  const size_t rounds = 1ull << nSetBits;
  for (uint64_t i = 0; i < rounds; ++i) {
    prepareStatus(status, i);
    care |= computeCare(builder, status, careView,
                        pivot, k, branches, cellTables);
    if (kitty::is_const0(~care)) {
      break;
    }
  }
  return care;
}

//----------------------------------------------------------------------------//
// Branches
//----------------------------------------------------------------------------//

static bool collectBranchesRecursively(SubnetBuilder &builder,
                                       size_t idx,
                                       uint64_t &status,
                                       std::vector<size_t> &branches) {

  if (builder.isMarked(idx)) {
    return false;
  }

  while (builder.getCell(idx).isBuf()) {
    builder.mark(idx);
    const auto &link = builder.getLink(idx, 0);
    idx = link.idx;
    if (builder.isMarked(idx)) {
      return false;
    }
  }

  const auto &cell = builder.getCell(idx);
  if (cell.isZero() || cell.isOne()) {
    builder.mark(idx);
    return false;
  }

  if (builder.getSessionID(idx) < builder.getSessionID() - 2) {
    builder.mark(idx);
    size_t branchID = branches.size();
    if (branchID == maxBranches) {
      return true;
    }
    branches.push_back(idx);
    status |= 1ull << (32 + branchID);
    return false;
  }
  builder.mark(idx);

  for (const auto &link : builder.getLinks(idx)) {
    if (collectBranchesRecursively(builder, link.idx, status, branches)) {
      return true;
    }
  }

  return false;
}

static uint64_t collectBranches(SubnetBuilder &builder,
                                const SubnetView &view,
                                const std::vector<size_t> &roots,
                                std::vector<size_t> &branches) {

  uint64_t status = 0;
  bool overflow = false;

  builder.startSession();

  for (const auto &pivot : view.getOutputs()) {
    builder.mark(pivot);
  }
  for (const auto &input : view.getInputs()) {
    builder.mark(input);
  }
  for (const auto &r : roots) {
    overflow |= collectBranchesRecursively(builder, r, status, branches);
    if (overflow) {
      builder.endSession();
      return -1;
    }
  }

  builder.endSession();

  return status;
}

//----------------------------------------------------------------------------//
// Roots
//----------------------------------------------------------------------------//

static void collectRootsRecursively(SubnetBuilder &builder,
                                    size_t idx,
                                    std::vector<size_t> &roots) {

  if (builder.isMarked(idx)) {
    return;
  }

  builder.mark(idx);

  bool allMarked = true;
  for (const auto &fanout : builder.getFanouts(idx)) {
    if (builder.getSessionID(fanout) < (builder.getSessionID() - 1)) {
      allMarked = false;
      break;
    }
  }

  if (!allMarked || builder.getCell(idx).isOut()) {
    roots.push_back(idx);
    return;
  }

  for (const auto &fanout : builder.getFanouts(idx)) {
    collectRootsRecursively(builder, fanout, roots);
  }
}

static std::vector<size_t> collectRoots(SubnetBuilder &builder, size_t pivot) {
  std::vector<size_t> roots;
  builder.startSession();
  collectRootsRecursively(builder, pivot, roots);
  builder.endSession();
  return roots;
}

//----------------------------------------------------------------------------//
// Transitive fanouts marking
//----------------------------------------------------------------------------//

static void markEntryTFORecursively(SubnetBuilder &builder,
                                    size_t idx,
                                    unsigned maxDepth) {

  if (builder.isMarked(idx)) {
    return;
  }

  builder.mark(idx);
  if (builder.getDepth(idx) >= maxDepth) {
    return;
  }

  for (const auto &fanout : builder.getFanouts(idx)) {
    markEntryTFORecursively(builder, fanout, maxDepth);
  }
}

static void markCutTFO(SubnetBuilder &builder,
                       const SubnetView &view,
                       unsigned maxLevels) {

  assert(!view.getOutputs().empty());
  builder.startSession();
  const size_t pivot = view.getOut(0);
  const size_t maxDepth = builder.getDepth(pivot) + maxLevels;

  // Mark nodes bypassing pivot (try to find reconvergence).
  builder.mark(pivot);

  for (const auto &input : view.getInputs()) {
    markEntryTFORecursively(builder, input, maxDepth);
  }

  builder.endSession();
}

//----------------------------------------------------------------------------//
// Transform
//----------------------------------------------------------------------------//

static void getTarget(const SubnetBuilder *builderPtr,
                      const TruthTable &care,
                      size_t pivot,
                      size_t arity,
                      TruthTable &onset,
                      TruthTable &offset) {

  if (arity > 6) {
    onset = utils::getTruthTable<utils::TTn>(*builderPtr, pivot) & care;
    offset = ~utils::getTruthTable<utils::TTn>(*builderPtr, pivot) & care;
    return;
  }
  const auto tt = utils::getTruthTable<utils::TT6>(*builderPtr, pivot);

  onset = utils::convertTruthTable<utils::TT6>(tt, arity) & care;
  offset = ~utils::convertTruthTable<utils::TT6>(tt, arity) & care;
}

static bool isAcceptable(const SubnetBuilder *builderPtr, size_t pivot) {
  const auto &cell = builderPtr->getCell(pivot);
  if (cell.isIn() || cell.isOne() || cell.isZero()) {
    return false;
  }

  if (builderPtr->getFanouts(pivot).size() == 1) {
    const auto fanout = builderPtr->getFanouts(pivot)[0];
    if (builderPtr->getCell(fanout).isBuf()) {
      return false;
    }
  }

  return true;
}

void Resubstitutor::transform(const SubnetBuilderPtr &builder) const {
  SubnetBuilder *builderPtr = builder.get();
  builderPtr->enableFanouts();

  CellTables cellTables;
  if (cutSize > 6) {
    cellTables.reserve(builderPtr->getCellNum());
  }

  for (SafePasser iter = builderPtr->begin();
      iter != builderPtr->end() && !builderPtr->getCell(*iter).isOut();
      ++iter) {

    const auto pivot = *iter;

    if (!isAcceptable(builderPtr, pivot)) {
      continue;
    }

    cellTables.clear();
    const auto view = getReconvergenceCut(*builderPtr, pivot, cutSize);

    // Mark TFO of reconvergence cut bypassing pivot.
    markCutTFO(*builderPtr, view, maxLevels);
  
    const std::vector<size_t> roots = collectRoots(*builderPtr, pivot);
    if ((roots.size() == 1) && (roots[0] == pivot)) {
      continue;
    }

    // Collect branches (new inputs for "don't care" evaluation).
    std::vector<size_t> branches;
    uint64_t status = collectBranches(*builderPtr, view, roots, branches);
    if (status == (uint64_t)-1) {
      continue;
    }

    simulateCone(*builderPtr, view, cellTables);
    cellTables.setPivotID(cellTables.size() - 1);

    const auto care = computeCare(*builderPtr, view, roots,
                                  branches, status, cellTables);

    const size_t arity = view.getInNum();
    TruthTable onset;
    TruthTable offset;
    getTarget(builderPtr, care, pivot, arity, onset, offset);

    if (makeConstResubstitution(iter, view, onset, offset)) {
      continue;
    }

    IdxMap mffcMap;
    const auto mffc = getMffc(*builderPtr, pivot, view.getInputs(), mffcMap);

    Divisors divs;
    divs.reserveUnates(maxDivisors);
    divs.reservePairs(maxDivisorsPairs);

    if (makeZeroResubstitution(*builderPtr, iter, view, divs, onset,
                               offset, cellTables, mffcMap)) {
      continue;
    }

    const size_t maxGain = Subnet::get(mffc).size() - mffcMap.size();
    if (maxGain == 1 || makeOneResubstitution(*builderPtr, iter, view, divs,
                                              onset, offset)) {
      continue;
    }

    DivisorsTT divsTT;
    divsTT.reserve(maxDivisorsPairs);

    if (maxGain == 2 || makeTwoResubstitution(*builderPtr, iter, view, divs,
                                              divsTT, onset, offset)) {
      continue;
    }

    if (maxGain == 3 || makeThreeResubstitution(*builderPtr, iter, view, divs,
                                                divsTT, onset, offset)) {
      continue;
    }
  }
}

} // namespace eda::gate::optimizer
