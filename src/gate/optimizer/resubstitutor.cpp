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
static_assert(maxBranches <= 16);

//----------------------------------------------------------------------------//
// Data types
//----------------------------------------------------------------------------//

using Cell             = eda::gate::model::Subnet::Cell;
using IdxMap           = std::unordered_map<model::EntryID, model::EntryID>;
using InOutMapping     = eda::gate::model::InOutMapping;
using Link             = eda::gate::model::Subnet::Link;
using LinkList         = eda::gate::model::Subnet::LinkList;
using SubnetBuilder    = eda::gate::model::SubnetBuilder;
using SubnetView       = eda::gate::model::SubnetView;
using SubnetViewWalker = eda::gate::model::SubnetViewWalker;
using Symbol           = eda::gate::model::CellSymbol;
using TruthTable       = eda::util::TruthTable;
using TruthTables      = std::vector<TruthTable>;

// Shortcuts
using TTn = TruthTable;
using TT6 = util::TT6;

//----------------------------------------------------------------------------//
// Data structures
//----------------------------------------------------------------------------//

enum DivisorType {
  Positive,
  Negative,
  Binate
};

struct Divisor {
  Divisor() = delete;
  Divisor(uint32_t divisor) = delete;
  Divisor(const Divisor &div) = default;
  Divisor(model::EntryID idx, bool inv): idx(idx), inv(inv) {}

  uint32_t idx : 31;
  uint32_t inv : 1;
};
static_assert(sizeof(Divisor) == 4);

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

  const TruthTable &getTruthTable(DivisorType pair, model::EntryID i) const {
    switch (pair) {
      case DivisorType::Positive: return positiveTTs[i];
      case DivisorType::Negative: return negativeTTs[i];

      default: assert(false && "Unsupported divisor type!");
    }
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

  size_t sizeUnate(DivisorType unate) const {
    switch (unate) {
      case DivisorType::Positive: return posUnate.size();
      case DivisorType::Negative: return negUnate.size();
      case DivisorType::Binate  : return   binate.size();

      default: assert(false && "Unsupported divisor type!");
    }
  }
  size_t sizePair(DivisorType pair) const {
    switch (pair) {
      case DivisorType::Positive: return pairPos.size();
      case DivisorType::Negative: return pairNeg.size();

      default: assert(false && "Unsupported divisor type!");
    }
  }

  Divisor getDivisor(DivisorType unate, model::EntryID i) const {
    switch (unate) {
      case DivisorType::Positive: return posUnate[i];
      case DivisorType::Negative: return negUnate[i];
      case DivisorType::Binate  : return   binate[i];

      default: assert(false && "Unsupported divisor type!");
    }
  }

  DivisorsPair getDivisorsPair(DivisorType pair, model::EntryID i) const {
    switch (pair) {
      case DivisorType::Positive: return pairPos[i];
      case DivisorType::Negative: return pairNeg[i];

      default: assert(false && "Unsupported divisor type!");
    }
  }

  void erase(DivisorType unate, model::EntryID i) {
    switch (unate) {
      case DivisorType::Positive: posUnate.erase(posUnate.begin() + i); break;
      case DivisorType::Negative: negUnate.erase(negUnate.begin() + i); break;
      case DivisorType::Binate  :   binate.erase(  binate.begin() + i); break;

      default: assert(false && "Unsupported divisor type!");
    }
  }

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

  void setPivotID(model::EntryID idx) { pivotID = idx; }

  void reserve(size_t nCells) {
    tables.reserve(nCells);
  }

  void pushBranch(const TruthTable &table) {
    if (firstBranchID == (model::EntryID)-1) {
      firstBranchID = tables.size();
    }
    nBranches++;
    tables.push_back(table);
  }
  void pushBranch(TruthTable &&table) {
    if (firstBranchID == (model::EntryID)-1) {
      firstBranchID = tables.size();
    }
    nBranches++;
    tables.push_back(table);
  }

  void pushOuter(const TruthTable &table) {
    if (firstOuterID == (model::EntryID)-1) {
      firstOuterID = tables.size();
    }
    nOuters++;
    tables.push_back(table);
  }
  void pushOuter(TruthTable &&table) {
    if (firstOuterID == (model::EntryID)-1) {
      firstOuterID = tables.size();
    }
    nOuters++;
    tables.push_back(table);
  }

  void setBranchTT(model::EntryID pos, const TruthTable &table) {
    assert(pos < nBranches);
    tables[firstBranchID + pos] = table;
  }
  void setBranchTT(model::EntryID pos, TruthTable &&table) {
    assert(pos < nBranches);
    tables[firstBranchID + pos] = table;
  }

  void setOuterTT(model::EntryID pos, const TruthTable &table) {
    assert(pos < nOuters);
    tables[firstOuterID + pos] = table;
  }
  void setOuterTT(model::EntryID pos, TruthTable &&table) {
    assert(pos < nOuters);
    tables[firstOuterID + pos] = table;
  }

  void invertPivotTT() {
    assert(pivotID < tables.size());
    tables[pivotID] = ~tables[pivotID];
  }

private:
  TruthTables tables;
  model::EntryID firstBranchID = -1;
  model::EntryID firstOuterID = -1;
  model::EntryID nBranches = 0;
  model::EntryID nOuters = 0;
  model::EntryID pivotID = -1;
};

//----------------------------------------------------------------------------//
// Convenient methods
//----------------------------------------------------------------------------//

static bool isConst0And(const TT6 &tt1, const TTn &tt2) {
  return (*tt2.begin() & tt1) == 0;
}

static bool isConst0And(const TTn &tt1, const TTn &tt2) {
  return kitty::is_const0(tt1 & tt2);
}

static uint32_t countNodes(const SubnetView &view) {
  uint32_t counter = 0;
  SubnetViewWalker walker(view);

  walker.run([&counter](SubnetBuilder &builder,
                        const bool isIn,
                        const bool isOut,
                        const model::EntryID i) -> bool {
      if (isIn) {
        return true;
      }
      counter++;
      return true;
    });

  return counter;
}

static void buildFromDivisor(const SubnetBuilder &builder,
                             SubnetBuilder &rhs,
                             model::EntryID idx,
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
    auto idNew = oldToNew.at(links[i].idx);
    links[i].idx = idNew;
  }
  oldToNew[idx] = rhs.addCell(symbol, links).idx;
}

static Link addCell(SubnetBuilder &builder,
                    Link link1,
                    Link link2,
                    DivisorType unate) {

   switch (unate) {
    case DivisorType::Positive:
      return builder.addCell(Symbol::OR, link1, link2);
    case DivisorType::Negative:
      return builder.addCell(Symbol::AND, link1, link2);
    
    default: assert(false && "Unsupported divisor type!");
  }
}

static void removeDeepDivisors(const SubnetBuilder &builder,
                               DivisorType unate,
                               Divisors &divs,
                               model::EntryID pivot,
                               uint16_t delta) {

  const auto maxDepth = builder.getDepth(pivot) - delta;
  size_t i = divs.sizeUnate(unate);
  while (i) {
    --i;
    const auto divID = divs.getDivisor(unate, i).idx;
    if (builder.getDepth(divID) > maxDepth) {
      divs.erase(unate, i);
    }
  }
}

//----------------------------------------------------------------------------//
// Maximum fanout-free cone marking
//----------------------------------------------------------------------------//

static void markMffcRecursively(SubnetBuilder &builder, model::EntryID idx) {
  if (builder.isMarked(idx)) {
    return;
  }

  builder.mark(idx);

  for (const auto &link : builder.getLinks(idx)) {
    markMffcRecursively(builder, link.idx);
  }
}

static uint32_t markMffc(SubnetBuilder &builder,
                         const SubnetView &view,
                         const model::EntryIDList &mffc) {

  builder.startSession();

  for (const auto &in : mffc) {
    builder.mark(in);
  }
  markMffcRecursively(builder, view.getOut(0));

  const auto res = builder.getSessionID();
  builder.endSession();

  return res;
}

//----------------------------------------------------------------------------//
// Resubstitution making
//----------------------------------------------------------------------------//

static SubnetBuilder initResubstitution(const SubnetView &view,
                                        InOutMapping &iomapping,
                                        IdxMap &oldToNew) {

  SubnetBuilder rhs;

  iomapping.inputs = view.getInputs();
  iomapping.outputs = view.getOutputs();

  for (size_t i = 0; i < iomapping.getInNum(); ++i) {
    oldToNew[iomapping.getIn(i)] = rhs.addInput().idx;
  }

  return rhs;
}

static bool makeResubstitution(const SubnetBuilder &builder,
                               SafePasser &iter,
                               const SubnetBuilder &rhs,
                               const InOutMapping &iomapping) {

  if (builder.evaluateReplace(rhs, iomapping).size < 0) {
    return false;
  }
  iter.replace(rhs, iomapping);
  return true;
}

static bool makeZeroResubstitution(const SubnetBuilder &builder,
                                   SafePasser &iter,
                                   const SubnetView &view,
                                   Divisor div) {

  InOutMapping iomapping;
  IdxMap oldToNew;
  SubnetBuilder rhs = initResubstitution(view, iomapping, oldToNew);

  buildFromDivisor(builder, rhs, div.idx, oldToNew);

  Link link(oldToNew.at(div.idx), div.inv);
  rhs.addOutput(link);

  return makeResubstitution(builder, iter, rhs, iomapping);
}

static bool makeOneResubstitution(const SubnetBuilder &builder,
                                  SafePasser &iter,
                                  const SubnetView &view,
                                  Divisor div1,
                                  Divisor div2,
                                  DivisorType unate) {

  InOutMapping iomapping;
  IdxMap oldToNew;
  SubnetBuilder rhs = initResubstitution(view, iomapping, oldToNew);

  buildFromDivisor(builder, rhs, div1.idx, oldToNew);
  buildFromDivisor(builder, rhs, div2.idx, oldToNew);

  Link link1(oldToNew.at(div1.idx), div1.inv);
  Link link2(oldToNew.at(div2.idx), div2.inv);

  const Link link = addCell(rhs, link1, link2, unate);

  rhs.addOutput(link);

  return makeResubstitution(builder, iter, rhs, iomapping);
}

static bool makeTwoResubstitution(const SubnetBuilder &builder,
                                  SafePasser &iter,
                                  const SubnetView &view,
                                  DivisorsPair divPair,
                                  Divisor div2,
                                  DivisorType unate) {

  InOutMapping iomapping;
  IdxMap oldToNew;
  SubnetBuilder rhs = initResubstitution(view, iomapping, oldToNew);

  const Divisor divF = divPair.first;
  const Divisor divS = divPair.second;

  buildFromDivisor(builder, rhs, divF.idx, oldToNew);
  buildFromDivisor(builder, rhs, divS.idx, oldToNew);
  buildFromDivisor(builder, rhs, div2.idx, oldToNew);

  Link linkF(oldToNew.at(divF.idx), divF.inv);
  Link linkS(oldToNew.at(divS.idx), divS.inv);
  Link link2(oldToNew.at(div2.idx), div2.inv);
  Link link1 = rhs.addCell(Symbol::AND, linkF, linkS);

  link1.inv ^= divPair.inv;
  const Link link = addCell(rhs, link1, link2, unate);
  rhs.addOutput(link);
  return makeResubstitution(builder, iter, rhs, iomapping);
}

static bool makeThreeResubstitution(const SubnetBuilder &builder,
                                    SafePasser &iter,
                                    const SubnetView &view,
                                    DivisorsPair pair1,
                                    DivisorsPair pair2,
                                    DivisorType unate) {

  InOutMapping iomapping;
  IdxMap oldToNew;
  SubnetBuilder rhs = initResubstitution(view, iomapping, oldToNew);

  const Divisor divF1 = pair1.first;
  const Divisor divS1 = pair1.second;
  const Divisor divF2 = pair2.first;
  const Divisor divS2 = pair2.second;

  buildFromDivisor(builder, rhs, divF1.idx, oldToNew);
  buildFromDivisor(builder, rhs, divS1.idx, oldToNew);
  buildFromDivisor(builder, rhs, divF2.idx, oldToNew);
  buildFromDivisor(builder, rhs, divS2.idx, oldToNew);

  Link linkF1(oldToNew.at(divF1.idx), divF1.inv);
  Link linkS1(oldToNew.at(divS1.idx), divS1.inv);
  Link linkF2(oldToNew.at(divF2.idx), divF2.inv);
  Link linkS2(oldToNew.at(divS2.idx), divS2.inv);

  Link link1 = rhs.addCell(Symbol::AND, linkF1, linkS1);
  Link link2 = rhs.addCell(Symbol::AND, linkF2, linkS2);

  link1.inv ^= pair1.inv;
  link2.inv ^= pair2.inv;

  const Link link = addCell(rhs, link1, link2, unate);
  rhs.addOutput(link);
  return makeResubstitution(builder, iter, rhs, iomapping);
}

//----------------------------------------------------------------------------//
// Divisors classifications
//----------------------------------------------------------------------------//

template <typename TT>
static std::pair<bool, Divisor> classifyDivisor(Divisor div,
                                                Divisors &divs,
                                                const TT &table,
                                                const TruthTable &onset,
                                                const TruthTable &offset) {

  bool positive = false;
  bool negative = false;

  if (isConst0And(table, offset)) {
    divs.addPositive(div);
    positive = true;
  }
  if (isConst0And(~table, onset)) {
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

template <typename TT>
static std::pair<bool, Divisor> classifyDivisor(model::EntryID idx,
                                                Divisors &divs,
                                                const TT &table,
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

template <typename TT>
static void classifyBinatePair(const TT &table,
                               const DivisorsPair &divPair,
                               Divisors &divs,
                               DivisorsTT &divsTT,
                               const TruthTable &onset,
                               const TruthTable &offset) {

  const size_t arity = onset.num_vars();

  if (isConst0And(table, offset)) {
    divs.addPositive(divPair);
    divsTT.addPositiveTT(util::convertTruthTable<TT>(table, arity));
  } else if (isConst0And(~table, offset)) {
    divs.addPositive(~divPair);
    divsTT.addPositiveTT(util::convertTruthTable<TT>(~table, arity));
  } else if (isConst0And(~table, onset)) {
    divs.addNegative(divPair);
    divsTT.addNegativeTT(util::convertTruthTable<TT>(table, arity));
  } else if (isConst0And(table, onset)) {
    divs.addNegative(~divPair);
    divsTT.addNegativeTT(util::convertTruthTable<TT>(~table, arity));
  }
}

template <typename TT>
static void classifyBinatePair(const TT &tt1,
                               const TT &tt2,
                               const DivisorsPair &divPair,
                               Divisors &divs,
                               DivisorsTT &divsTT,
                               const TruthTable &onset,
                               const TruthTable &offset) {

  const Divisor div1(divPair.first);
  const Divisor div2(divPair.second);

  if (!div1.inv && !div2.inv) {
    classifyBinatePair(tt1 & tt2, divPair, divs, divsTT, onset, offset);
  } else if (!div1.inv && div2.inv) {
    classifyBinatePair(tt1 & ~tt2, divPair, divs, divsTT, onset, offset);
  } else if (div1.inv && !div2.inv) {
    classifyBinatePair(~tt1 & tt2, divPair, divs, divsTT, onset, offset);
  } else if (div1.inv && div2.inv) {
    classifyBinatePair(~tt1 & ~tt2, divPair, divs, divsTT, onset, offset);
  }
}

static void classifyBinatePairs(SubnetBuilder &builder,
                                const SubnetView &view,
                                Divisors &divs,
                                DivisorsTT &divsTT,
                                const TruthTable &onset,
                                const TruthTable &offset) {

  builder.startSession();
  const size_t arity = view.getInNum();
  for (size_t i = 0; i < divs.sizeUnate(Binate); ++i) {
    for (size_t j = i + 1; j < divs.sizeUnate(Binate); ++j) {
      const Divisor div1(divs.getDivisor(Binate, i));
      const Divisor div2(divs.getDivisor(Binate, j));

      if (div1.idx == div2.idx) {
        continue;
      }

      builder.mark(div1.idx);
      builder.mark(div2.idx);

      const DivisorsPair divPair(div1, div2, false);

      if (arity > 6) {
        const TTn &tt1 = util::getTruthTable<TTn>(builder, div1.idx);
        const TTn &tt2 = util::getTruthTable<TTn>(builder, div2.idx);

        classifyBinatePair(tt1, tt2, divPair, divs, divsTT, onset, offset);
      } else {
        const TT6 &tt1 = util::getTruthTable<TT6>(builder, div1.idx);
        const TT6 &tt2 = util::getTruthTable<TT6>(builder, div2.idx);

        classifyBinatePair(tt1, tt2, divPair, divs, divsTT, onset, offset);
      }

      if (divs.nPairs() > maxDivisorsPairs) {
        builder.endSession();
        return;
      }
    }
  }
  builder.endSession();
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
                                                uint32_t mffcID,
                                                model::EntryID idx) {

  if (builder.isMarked(idx) || (builder.getSessionID(idx) == mffcID)) {
    return std::make_pair(false, Divisor(0, 0));
  }
  const auto maxDepth = builder.getDepth(view.getOut(0));
  if ((builder.getDepth(idx) > maxDepth) || (divs.nUnates() >= maxDivisors)) {
    return std::make_pair(false, Divisor(0, 0));
  }

  for (const auto &link : builder.getLinks(idx)) {
    if (!builder.isMarked(link.idx)) {
      return std::make_pair(false, Divisor(0, 0));
    }
  }

  if (builder.getSessionID(idx) != (builder.getSessionID() - 1)) {
    const auto arity = view.getInNum();
    auto res = std::make_pair(false, Divisor(0, 0));
    builder.mark(idx);

    if (arity > 6) {
      const TTn &tt = util::getTruthTable<TTn>(builder, arity, idx, false, 0);
      cellTables.push(tt);
      util::setTruthTable<TTn>(builder, idx, cellTables.back());
      res = classifyDivisor(idx, divs, tt, onset, offset);
    } else {
      const TT6 tt = util::getTruthTable<TT6>(builder, arity, idx, false, 0);
      util::setTruthTable<TT6>(builder, idx, tt);
      res = classifyDivisor(idx, divs, tt, onset, offset);
    }

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

static bool getSideDivisors(SubnetBuilder &builder,
                            SafePasser &iter,
                            const SubnetView &view,
                            Divisors &divs,
                            const TruthTable &onset,
                            const TruthTable &offset,
                            CellTables &cellTables,
                            uint32_t mffcID) {

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
        return makeZeroResubstitution(builder, iter, view, res.second);
      }
    }
  }

  builder.endSession();
  return false;
}

//----------------------------------------------------------------------------//
// Divisors collecting from the inputs of the mffc to the cut (part of the cone)
//----------------------------------------------------------------------------//

static std::pair<bool, Divisor> addInnerDivisor(const SubnetBuilder &builder,
                                                Divisors &divs,
                                                model::EntryID idx,
                                                uint16_t arity,
                                                const TruthTable &onset,
                                                const TruthTable &offset) {

  if (arity > 6) {
    const TTn &tt = util::getTruthTable<TTn>(builder, idx);
    return classifyDivisor(idx, divs, tt, onset, offset);
  }
  const TT6 &tt = util::getTruthTable<TT6>(builder, idx);
  return classifyDivisor(idx, divs, tt, onset, offset);
}

static std::pair<bool, Divisor> getInnerDivisors(SubnetBuilder &builder,
                                                 Divisors &divs,
                                                 model::EntryID idx,
                                                 uint16_t arity,
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

static bool getInnerDivisors(SubnetBuilder &builder,
                             SafePasser &iter,
                             const SubnetView &view,
                             Divisors &divs,
                             const TruthTable &onset,
                             const TruthTable &offset,
                             const model::EntryIDList &mffc) {

  builder.startSession();

  const auto &inputs = view.getInputs();
  const auto arity = view.getInNum();

  // Add the cut.
  for (uint16_t i = 0; i < arity; ++i) {
    builder.mark(inputs[i]);
    const auto res = addInnerDivisor(
        builder, divs, inputs[i], arity, onset, offset);

    if (res.first) {
      builder.endSession();
      return makeZeroResubstitution(builder, iter, view, res.second);
    }
  }
  // Get divisors from the inputs of the mffc to cut.
  for (size_t i = 0; i < mffc.size(); ++i) {
    const auto res = getInnerDivisors(
        builder, divs, mffc[i], arity, onset, offset);
    
    if (res.first) {
      builder.endSession();
      return makeZeroResubstitution(builder, iter, view, res.second);
    }
  }

  builder.endSession();
  return false;
}

//----------------------------------------------------------------------------//
// Divisors collecting (inner + side)
//----------------------------------------------------------------------------//

static bool getDivisors(SubnetBuilder &builder,
                        SafePasser &iter,
                        const SubnetView &view,
                        Divisors &divs,
                        const TruthTable &onset,
                        const TruthTable &offset,
                        CellTables &tables,
                        const model::EntryIDList &mffc) {

  const auto id = markMffc(builder, view, mffc);
  if (getInnerDivisors(builder, iter, view, divs, onset, offset, mffc)) {
    return true;
  }
  return getSideDivisors(builder, iter, view, divs, onset, offset, tables, id);
}

//----------------------------------------------------------------------------//
// Resubstitution checking
//----------------------------------------------------------------------------//

template <typename TT>
static bool checkUnates(const TT &tt1,
                        const TT &tt2,
                        const TruthTable &target,
                        DivisorType unate) {

  switch (unate) {
    case DivisorType::Positive: return isConst0And(~(tt1 | tt2), target);
    case DivisorType::Negative: return isConst0And(tt1 & tt2, target);

    default: assert(false && "Unsupported divisor type!");
  }
}

inline bool checkUnates(const TTn &tt1,
                        const TT6 &tt2,
                        const TTn &target,
                        DivisorType unate) {

  return checkUnates(*tt1.begin(), tt2, target, unate);
}

template <typename TT1, typename TT2>
static bool checkUnates(const TT1 &tt1,
                        const TT2 &tt2,
                        bool inv1,
                        bool inv2,
                        const TruthTable &target,
                        DivisorType unate) {

  if (!inv1 && !inv2) {
    return checkUnates(tt1, tt2, target, unate);
  } 
  if (!inv1 && inv2) {
    return checkUnates(tt1, ~tt2, target, unate);
  }
  if (inv1 && !inv2) {
    return checkUnates(~tt1, tt2, target, unate);
  }
  return checkUnates(~tt1, ~tt2, target, unate);
}

static bool checkUnates(SubnetBuilder &builder,
                        SafePasser &iter,
                        const SubnetView &view,
                        const Divisors &divs,
                        const TruthTable &target,
                        DivisorType unate,
                        uint16_t arity) {

  builder.startSession();
  for (size_t i = 0; i < divs.sizeUnate(unate); ++i) {
    for (size_t j = i + 1; j < divs.sizeUnate(unate); ++j) {
      const Divisor div1 = divs.getDivisor(unate, i);
      const Divisor div2 = divs.getDivisor(unate, j);

      builder.mark(div1.idx);
      builder.mark(div2.idx);

      bool success = false;

      if (arity <= 6) {
        const TT6 &tt1 = util::getTruthTable<TT6>(builder, div1.idx);
        const TT6 &tt2 = util::getTruthTable<TT6>(builder, div2.idx);
        success = checkUnates(tt1, tt2, div1.inv, div2.inv, target, unate);
      } else {
        const TTn &tt1 = util::getTruthTable<TTn>(builder, div1.idx);
        const TTn &tt2 = util::getTruthTable<TTn>(builder, div2.idx);
        success = checkUnates(tt1, tt2, div1.inv, div2.inv, target, unate);
      }

      if (success) {
        builder.endSession();
        return makeOneResubstitution(builder, iter, view, div1, div2, unate);
      }
    }
  }
  builder.endSession();
  return false;
}

static bool checkUnatePair(SubnetBuilder &builder,
                           SafePasser &iter,
                           const SubnetView &view,
                           const Divisors &divs,
                           const DivisorsTT &divsTT,
                           const TruthTable &target,
                           DivisorType unate,
                           uint16_t arity) {

  builder.startSession();
  for (size_t i = 0; i < divs.sizePair(unate); ++i) {
    for (size_t j = 0; j < divs.sizeUnate(unate); ++j) {
      const Divisor div2 = divs.getDivisor(unate, j);

      builder.mark(div2.idx);

      bool success = false;

      const TruthTable &tt1 = divsTT.getTruthTable(unate, i);
      if (arity <= 6) {
        const TT6 &tt2 = util::getTruthTable<TT6>(builder, div2.idx);
        success = checkUnates(tt1, tt2, false, div2.inv, target, unate);
      } else {
        const TTn &tt2 = util::getTruthTable<TTn>(builder, div2.idx);
        success = checkUnates(tt1, tt2, false, div2.inv, target, unate);
      }

      if (success) {
        builder.endSession();
        const DivisorsPair divPair = divs.getDivisorsPair(unate, i);
        return makeTwoResubstitution(builder, iter, view, divPair, div2, unate);
      }
    }
  }
  builder.endSession();
  return false;
}

static bool checkPairs(SubnetBuilder &builder,
                       SafePasser &iter,
                       const SubnetView &view,
                       const Divisors &divs,
                       const DivisorsTT &divsTT,
                       const TruthTable &target,
                       DivisorType pair) {

  for (size_t i = 0; i < divs.sizePair(pair); ++i) {
    for (size_t j = i + 1; j < divs.sizePair(pair); ++j) {
      const TruthTable &tt1 = divsTT.getTruthTable(pair, i);
      const TruthTable &tt2 = divsTT.getTruthTable(pair, j);

      if (checkUnates(tt1, tt2, false, false, target, pair)) {
        const DivisorsPair pair1 = divs.getDivisorsPair(pair, i);
        const DivisorsPair pair2 = divs.getDivisorsPair(pair, j);
        return makeThreeResubstitution(builder, iter, view, pair1, pair2, pair);
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------//
// Resubstitutions (const, zero, one, two, three)
//----------------------------------------------------------------------------//

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

inline bool makeZeroResubstitution(SubnetBuilder &builder,
                                   SafePasser &iter,
                                   const SubnetView &view,
                                   Divisors &divs,
                                   const TruthTable &onset,
                                   const TruthTable &offset,
                                   CellTables &tables,
                                   const model::EntryIDList &mffc) {

  return getDivisors(builder, iter, view, divs, onset, offset, tables, mffc);
}

static bool makeOneResubstitution(SubnetBuilder &builder,
                                  SafePasser &iter,
                                  const SubnetView &view,
                                  Divisors &divs,
                                  const TruthTable &onset,
                                  const TruthTable &offset,
                                  bool saveDepth) {

  const auto arity = view.getInNum();

  if (saveDepth) {
    removeDeepDivisors(builder, Negative, divs, view.getOut(0), 1);
    removeDeepDivisors(builder, Positive, divs, view.getOut(0), 1);
  }
  if (checkUnates(builder, iter, view, divs, offset, Negative, arity)) {
    return true;
  }
  return checkUnates(builder, iter, view, divs, onset, Positive, arity);
}

static bool makeTwoResubstitution(SubnetBuilder &builder,
                                  SafePasser &iter,
                                  const SubnetView &view,
                                  Divisors &divs,
                                  DivisorsTT &divsTT,
                                  const TruthTable &onset,
                                  const TruthTable &offset,
                                  bool saveDepth) {

  const auto arity = view.getInNum();

  if (saveDepth) {
    removeDeepDivisors(builder, Binate, divs, view.getOut(0), 2);
  }

  classifyBinatePairs(builder, view, divs, divsTT, onset, offset);

  if (checkUnatePair(builder, iter, view, divs,
                     divsTT, offset, Negative, arity)) {
    return true;
  }
  return checkUnatePair(builder, iter, view, divs,
                        divsTT, onset, Positive, arity);
}

static bool makeThreeResubstitution(SubnetBuilder &builder,
                                    SafePasser &iter,
                                    const SubnetView &view,
                                    Divisors &divs,
                                    DivisorsTT &divsTT,
                                    const TruthTable &onset,
                                    const TruthTable &offset) {

  if (checkPairs(builder, iter, view, divs, divsTT, offset, Negative)) {
    return true;
  }
  return checkPairs(builder, iter, view, divs, divsTT, onset, Positive);
}

//----------------------------------------------------------------------------//
// Simulations
//----------------------------------------------------------------------------//

static void simulateCone(SubnetBuilder &builder,
                         const SubnetView &view,
                         CellTables &cellTables) {

  const auto arity = view.getInNum();
  if (arity <= 6) {
    view.evaluateTruthTable();
  } else {
    SubnetViewWalker walker(view);
    size_t nIn = 0;

    walker.run([&cellTables, &nIn, arity](SubnetBuilder &builder,
                                          const bool isIn,
                                          const bool isOut,
                                          const model::EntryID i) -> bool {
      const auto tt = util::getTruthTable<TTn>(builder, arity, i, isIn, nIn++);
      cellTables.push(std::move(tt));
      util::setTruthTable<TTn>(builder, i, cellTables.back());
      return true; // Continue traversal.
    });
  }
}

static void invertPivotTT(SubnetBuilder &builder,
                          model::EntryID pivot,
                          CellTables &cellTables,
                          uint16_t arity) {

  if (arity > 6) {
    cellTables.invertPivotTT();
  } else {
    const auto inverted = ~util::getTruthTable<TT6>(builder, pivot);
    util::setTruthTable<TT6>(builder, pivot, inverted);
  }
}

static TruthTables evaluateRoots(SubnetBuilder &builder,
                                 const SubnetView &view,
                                 uint16_t arity,
                                 CellTables &cellTables) {

  TruthTables result(view.getOutNum());
  SubnetViewWalker walker(view);

  if (arity <= 6) {
    walker.run([arity](SubnetBuilder &builder,
                       const bool isIn,
                       const bool isOut,
                       const model::EntryID i) -> bool {
                                
      if (isIn) {
        return true; // Continue traversal.
      }
      const auto tt = util::getTruthTable<TT6>(builder, arity, i, isIn, 0);
      util::setTruthTable<TT6>(builder, i, tt);
      return true; // Continue traversal.
    });

    for (size_t i = 0; i < view.getOutNum(); ++i) {
      const auto tt = util::getTruthTable<TT6>(builder, view.getOut(i));
      result[i] = util::convertTruthTable<TT6>(tt, arity);
    }
  } else {
    size_t nOuter = 0;
    walker.run([&nOuter, &cellTables, arity](SubnetBuilder &builder,
                                             const bool isIn,
                                             const bool isOut,
                                             const model::EntryID i) -> bool {

      if (isIn) {
        return true; // Continue traversal.
      }
      auto tt = util::getTruthTable<TTn>(builder, arity, i, isIn, 0);
      cellTables.setOuterTT(nOuter++, std::move(tt));
      return true; // Continue traversal.
    });

    for (size_t i = 0; i < view.getOutNum(); ++i) {
      result[i] = util::getTruthTable<TTn>(builder, view.getOut(i));
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
                              model::EntryID pivot,
                              uint16_t arity,
                              const std::vector<model::EntryID> &branches,
                              CellTables &cellTables) {

  auto care = util::getZeroTruthTable<TTn>(arity);

  // Init branches.
  for (size_t i = 0; i < branches.size(); ++i) {
    if (arity <= 6) {
      const auto constant = ((status >> i) & 1ull) ?
          util::getOneTruthTable<TT6>(arity):
          util::getZeroTruthTable<TT6>(arity);

      util::setTruthTable<TT6>(builder, branches[i], constant);
    } else {
      auto constant = ((status >> i) & 1ull) ?
          util::getOneTruthTable<TTn>(arity):
          util::getZeroTruthTable<TTn>(arity);

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

static void markInnerRecursively(SubnetBuilder &builder, model::EntryID idx) {
  if (builder.isMarked(idx)) {
    return;
  }

  builder.mark(idx);

  for (const auto &link : builder.getLinks(idx)) {
    markInnerRecursively(builder, link.idx);
  }
}

static uint32_t markInner(SubnetBuilder &builder, const SubnetView &view) {
  builder.startSession();

  for (const auto &input : view.getInputs()) {
    builder.mark(input);
  }
  for (const auto &pivot : view.getOutputs()) {
    markInnerRecursively(builder, pivot);
  }

  const auto ret = builder.getSessionID();
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
                             model::EntryID idx,
                             uint32_t innerID) {

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
                              model::EntryID pivot,
                              const model::EntryIDList &roots,
                              const model::EntryIDList &branches,
                              uint16_t arity,
                              CellTables &cellTables,
                              uint32_t innerID) {

  builder.startSession();

  InOutMapping iomapping;
  if (arity > 6) {
    for (size_t i = 0; i < branches.size(); ++i) {
      auto zero = util::getZeroTruthTable<TTn>(arity);
      builder.mark(branches[i]);
      iomapping.inputs.push_back(branches[i]);
      cellTables.pushBranch(std::move(zero));
      util::setTruthTable<TTn>(builder, branches[i], cellTables.back());
    }
  } else {
    for (size_t i = 0; i < branches.size(); ++i) {
      const auto zero = util::getZeroTruthTable<TT6>(arity);
      builder.mark(branches[i]);
      iomapping.inputs.push_back(branches[i]);
      util::setTruthTable<TT6>(builder, branches[i], zero);
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
                          uint16_t arity) {

  SubnetViewWalker walker(view);

  walker.run([&cellTables, arity](SubnetBuilder &parent,
                                  const bool isIn,
                                  const bool isOut,
                                  const model::EntryID i) -> bool {

    if (isIn) {
      return true; // Continue traversal.
    }
    const auto zero = util::getZeroTruthTable<TTn>(arity);
    cellTables.pushOuter(std::move(zero));
    util::setTruthTable<TTn>(parent, i, cellTables.back());
    return true; // Continue traversal.
  });
}

static TruthTable computeCare(SubnetBuilder &builder,
                              const SubnetView &view,
                              const model::EntryIDList &roots,
                              const model::EntryIDList &branches,
                              uint64_t status,
                              CellTables &tables) {

  const auto k = view.getInNum();
  const auto pivot = view.getOut(0);

  // Mark inner nodes (from pivot to cut).
  const auto innerID = markInner(builder, view);

  const auto careView = getCareView(builder, pivot, roots, branches,
                                    k, tables, innerID);

  if (k > 6) {
    reserveOuters(careView, tables, k);
  }

  auto care = util::getZeroTruthTable<TTn>(k);

  const size_t nSetBits = countSetBits(status >> 32);
  const size_t rounds = 1ull << nSetBits;
  for (uint64_t i = 0; i < rounds; ++i) {
    prepareStatus(status, i);
    care |= computeCare(builder, status, careView, pivot, k, branches, tables);
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
                                       model::EntryID idx,
                                       uint64_t &status,
                                       model::EntryIDList &branches) {

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
                                const model::EntryIDList &roots,
                                model::EntryIDList &branches) {

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
                                    model::EntryID idx,
                                    model::EntryIDList &roots) {

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

static model::EntryIDList collectRoots(SubnetBuilder &builder,
                                       model::EntryID pivot) {

  model::EntryIDList roots;
  builder.startSession();
  collectRootsRecursively(builder, pivot, roots);
  builder.endSession();
  return roots;
}

//----------------------------------------------------------------------------//
// Transitive fanouts marking
//----------------------------------------------------------------------------//

static void markEntryTFORecursively(SubnetBuilder &builder,
                                    model::EntryID idx,
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
  const auto pivot = view.getOut(0);
  const auto maxDepth = builder.getDepth(pivot) + maxLevels;

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
                      model::EntryID pivot,
                      uint16_t arity,
                      TruthTable &onset,
                      TruthTable &offset) {

  if (arity > 6) {
    onset = util::getTruthTable<TTn>(*builderPtr, pivot) & care;
    offset = ~util::getTruthTable<TTn>(*builderPtr, pivot) & care;
    return;
  }
  const auto tt = util::getTruthTable<TT6>(*builderPtr, pivot);

  onset = util::convertTruthTable<TT6>(tt, arity) & care;
  offset = ~util::convertTruthTable<TT6>(tt, arity) & care;
}

static bool isAcceptable(const SubnetBuilder *builderPtr,
                         model::EntryID pivot) {

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

void Resubstitutor::transform(
    const std::shared_ptr<SubnetBuilder> &builder) const {

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
    const auto view = getReconvergentCut(*builderPtr, pivot, cutSize);

    // Mark TFO of reconvergent cut bypassing pivot.
    markCutTFO(*builderPtr, view, maxLevels);
  
    const model::EntryIDList roots = collectRoots(*builderPtr, pivot);
    if (((roots.size() == 1) && (roots[0] == pivot)) || roots.empty()) {
      continue;
    }

    // Collect branches (new inputs for "don't care" evaluation).
    model::EntryIDList branches;
    uint64_t status = collectBranches(*builderPtr, view, roots, branches);
    if (status == (uint64_t)-1) {
      continue;
    }

    simulateCone(*builderPtr, view, cellTables);
    cellTables.setPivotID(cellTables.size() - 1);

    const auto care = computeCare(*builderPtr, view, roots,
                                  branches, status, cellTables);

    const auto arity = view.getInNum();
    TruthTable onset;
    TruthTable offset;
    getTarget(builderPtr, care, pivot, arity, onset, offset);

    if (makeConstResubstitution(iter, view, onset, offset)) {
      continue;
    }

    const auto mffc = getMffc(*builderPtr, pivot, view.getInputs());

    Divisors divs;
    divs.reserveUnates(maxDivisors);
    divs.reservePairs(maxDivisorsPairs);

    if (makeZeroResubstitution(*builderPtr, iter, view, divs, onset,
                               offset, cellTables, mffc.getInputs())) {
      continue;
    }

    const auto maxGain = countNodes(mffc);
    bool skip = (maxGain == 1) && !zero;
    if (skip || makeOneResubstitution(*builderPtr, iter, view,
                                      divs, onset, offset, saveDepth)) {
      continue;
    }

    DivisorsTT divsTT;
    divsTT.reserve(maxDivisorsPairs);

    skip = ((maxGain == 2) && !zero) || (maxGain == 1);
    if (skip || makeTwoResubstitution(*builderPtr, iter, view, divs,
                                      divsTT, onset, offset, saveDepth)) {
      continue;
    }

    skip = ((maxGain == 3) && !zero) || (maxGain == 2);
    if (skip || makeThreeResubstitution(*builderPtr, iter, view, divs,
                                        divsTT, onset, offset)) {
      continue;
    }
  }
}

} // namespace eda::gate::optimizer
