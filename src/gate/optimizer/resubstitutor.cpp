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

static constexpr size_t maxBranchesNum = 8;
static constexpr size_t maxDivisorsNum = 150;
static_assert(maxBranchesNum <= 32);

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

struct Divisor {
  Divisor() = delete;
  Divisor(uint64_t divisor) = delete;
  Divisor(size_t idx, bool inv): idx(idx), inv(inv) {}

  uint64_t idx : 63;
  uint64_t inv : 1;
};
static_assert(sizeof(Divisor) == 8);

/// @brief Divisors storage.
class Divisors {
public:
  Divisors() = default;

  size_t size() const {
    return negUnate.size() +  posUnate.size() + binate.size();
  }

  void reserve(size_t nDivisors) {
    negUnate.reserve(nDivisors);
    posUnate.reserve(nDivisors);
    binate.reserve(nDivisors);

    pairNeg.reserve(nDivisors);
    pairPos.reserve(nDivisors);
  }

  void addPositive(Divisor div) { posUnate.push_back(div); }
  void addNegative(Divisor div) { negUnate.push_back(div); }
  void addBinate  (Divisor div) { binate.push_back(div);   }

private:
  std::vector<Divisor> negUnate;
  std::vector<Divisor> posUnate;
  std::vector<Divisor> binate;

  std::vector<std::pair<Divisor, Divisor>> pairNeg;
  std::vector<std::pair<Divisor, Divisor>> pairPos;
};

/// @brief Class is used when cut > 6 only (Non optimized case).
class CellTables {
public:
  CellTables() = default;

  const TruthTable &back() const { return tables.back(); }
  
  size_t size() const { return tables.size(); }

  void push(const TruthTable  &table) { tables.push_back(table); }
  void push(      TruthTable &&table) { tables.push_back(table); }

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
  if ((builder.getDepth(idx) > maxDepth) || (divs.size() >= maxDivisorsNum)) {
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

static bool makeZeroResubstitution(const SubnetBuilder &builder,
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
    return makeZeroResubstitution(builder, iter, view, res.second);
  }

  return false;
}

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
    if (branchID == maxBranchesNum) {
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

  for (SafePasser iter = builderPtr->begin();
      iter != builderPtr->end() && !builderPtr->getCell(*iter).isOut();
      ++iter) {

    const auto pivot = *iter;

    if (!isAcceptable(builderPtr, pivot)) {
      continue;
    }

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

    CellTables cellTables;
    const size_t arity = view.getInNum();
    if (arity > 6) {
      cellTables.reserve(builderPtr->getCellNum());
    }

    simulateCone(*builderPtr, view, cellTables);
    cellTables.setPivotID(cellTables.size() - 1);

    const auto care = computeCare(*builderPtr, view, roots,
                                  branches, status, cellTables);

    TruthTable onset;
    TruthTable offset;
    getTarget(builderPtr, care, pivot, arity, onset, offset);

    if (makeConstResubstitution(iter, view, onset, offset)) {
      continue;
    }

    IdxMap mffcMap;
    const auto mffc = getMffc(*builderPtr, pivot, view.getInputs(), mffcMap);

    Divisors divs;
    divs.reserve(maxDivisorsNum);

    if (makeZeroResubstitution(
        *builderPtr, iter, view, divs, onset, offset, cellTables, mffcMap)) {
      continue;
    }

    const size_t potentialGain = Subnet::get(mffc).size() - mffcMap.size();
    if (potentialGain == 1) {
      continue;
    }
  }
}

} // namespace eda::gate::optimizer
