//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/resynthesis/de_micheli.h"

namespace eda::gate::optimizer2::resynthesis {

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

using Link     = eda::gate::model::Subnet::Link;
using SubnetID = eda::gate::model::SubnetID;

//===----------------------------------------------------------------------===//
// Synthesis
//===----------------------------------------------------------------------===//

SubnetID DeMicheli::synthesize(const TruthTable &func, uint16_t maxArity) {
  assert(maxArity > 2 && "Arity of MAJ gate should be > 2");
  std::vector<TruthTable> divisors;
  std::vector<uint64_t> nOnes;
  std::vector<MajNode> tree;
  bool simplest = createDivisors(func, divisors, nOnes);
  if (simplest) {
    return buildSubnet(tree, divisors, true);
  }

  TruthTable care(divisors[0].num_vars());
  std::string bitsCare;
  bitsCare.assign(divisors[0].num_bits(), '1');
  kitty::create_from_binary_string(care, bitsCare);

  std::vector<MajNode> topNodes;
  createTopNodes(topNodes, tree, divisors, nOnes, care);
  if (!tree.empty()) {
    return buildSubnet(tree, divisors);
  }

  run(topNodes, tree, divisors);
  if (tree.empty()) {
    return eda::gate::model::OBJ_NULL_ID;
  }
  return buildSubnet(tree, divisors);
}

bool DeMicheli::createDivisors(const TruthTable &func,
                               std::vector<TruthTable> &divisors,
                               std::vector<uint64_t> &nOnes) const {

  const uint32_t nVars = func.num_vars();
  const uint64_t maxOnes = 1ull << nVars;
  divisors.clear();
  nOnes.clear();
  divisors.reserve(nVars * 2 + 2);
  nOnes.reserve(nVars * 2 + 2);
  // Input constants.
  // constant ZERO XNOR func = ~func
  divisors.emplace_back(~func);
  // constant ONE XNOR func = func
  divisors.emplace_back(func);
  // Count ones.
  nOnes.push_back(kitty::count_ones(divisors[0]));
  nOnes.push_back(maxOnes - nOnes[0]);
  if (!nOnes[0] || (nOnes[0] == maxOnes)) {
    return true;
  }
  // Input arguments.
  for (size_t i = 0; i < nVars; i++) {
    TruthTable divisor(nVars);
    create_nth_var(divisor, i);
    divisors.emplace_back(divisors[0] ^ divisor);
    const uint64_t count = kitty::count_ones(divisors.back());
    nOnes.push_back(count);
    divisors.emplace_back(divisors[1] ^ divisor);
    nOnes.push_back(maxOnes - count);
    if (!count || (count == maxOnes)) {
      return true;
    }
  }
  return false;
}

void DeMicheli::run(std::vector<MajNode> &topNodes,
                    std::vector<MajNode> &tree,
                    const std::vector<TruthTable> &divisors) const {

  for (auto &top : topNodes) {
    std::vector<MajNode> tmpTree;
    tmpTree.emplace_back(top);

    std::vector<Position> toExpand;
    toExpand.push_back({0, 0});
    toExpand.push_back({0, 1});
    toExpand.push_back({0, 2});

    bool isSuccess = isSuccessBuild(tmpTree, toExpand, divisors);

    if (isSuccess && (tree.empty() || (tree.size() < tmpTree.size()))) {
      tree = tmpTree;
    }
  }
}

//===----------------------------------------------------------------------===//
// Heuristics
//===----------------------------------------------------------------------===//

uint64_t DeMicheli::heuristicArg1(const std::vector<TruthTable> &divisors,
                                  const TruthTable &care) const {

  const size_t divisorsSize = divisors.size();
  uint64_t nOnes;
  uint64_t maxOnes = 0;
  uint64_t result = 0;
  for (size_t i = 0; i < divisorsSize; ++i) {
    nOnes = kitty::count_ones(divisors[i] & care);
    if (maxOnes < nOnes) {
      maxOnes = nOnes;
      result = i;
    }
  }
  return result;
}

uint64_t DeMicheli::heuristicArg2(uint64_t arg1,
                                  const std::vector<TruthTable> &divisors,
                                  const TruthTable &care) const {

  const size_t divisorsSize = divisors.size();
  const size_t invertedArg1 = getInverted(arg1);

  uint64_t nOnes;
  uint64_t maxOnes = 0;
  uint64_t result = 0;

  const TruthTable &table1 = divisors[arg1];
  const TruthTable &table2 = divisors[invertedArg1];

  for (size_t i = 0; i < divisorsSize; ++i) {
    if ((i == arg1) || (i == invertedArg1)) {
      continue;
    }
    nOnes = calculate(table1, table2, divisors[i], care);
    if (maxOnes < nOnes) {
      maxOnes = nOnes;
      result = i;
    }
  }
  return result;
}

uint64_t DeMicheli::heuristicArg3(uint64_t arg1,
                                  uint64_t arg2,
                                  const std::vector<TruthTable> &divisors,
                                  const TruthTable &care) const {

  const size_t divisorsSize = divisors.size();
  const size_t invertedArg1 = getInverted(arg1);
  const size_t invertedArg2 = getInverted(arg2);

  uint64_t nOnes;
  uint64_t maxOnes = 0;
  uint64_t result = 0;

  const TruthTable coveredOnes = divisors[arg1] ^ divisors[arg2];
  const TruthTable notCovered = divisors[invertedArg1] & divisors[invertedArg2];

  for (size_t i = 0; i < divisorsSize; ++i) {
    bool skip = (i == arg1) || (i == invertedArg1);
    skip = skip || (i == arg2) || (i == invertedArg2);
    if (skip) {
      continue;
    }
    nOnes = calculate(coveredOnes, notCovered, divisors[i], care);
    if (maxOnes < nOnes) {
      maxOnes = nOnes;
      result = i;
    }
  }
  return result;
}

uint64_t DeMicheli::calculate(const TruthTable &table1,
                              const TruthTable &table2,
                              const TruthTable &candidate,
                              const TruthTable &care) const {

  uint64_t summand1 = kitty::count_ones(table1 & candidate & care);
  uint64_t summand2 = kitty::count_ones(table2 & candidate & care);
  return summand1 + 2 * summand2;
}

//===----------------------------------------------------------------------===//
// Building a MAJ tree
//===----------------------------------------------------------------------===//

bool DeMicheli::isSuccessBuild(std::vector<MajNode> &tree,
                               std::vector<Position> &toExpand,
                               const std::vector<TruthTable> &divisors) const {

  const uint32_t nVars = tree[0].func.num_vars();
  while (!toExpand.empty() && tree.size() < BOUND) {
    uint64_t minUncovered = (1ull << nVars) + 1;
    uint64_t pos = 0;
    TruthTable oldFunc;
    TruthTable oldCare;

    for (uint64_t i = 0; i < toExpand.size(); ++i) {
      const uint64_t parent = toExpand[i].parent;
      const uint8_t arg = toExpand[i].arg;
      const int64_t idx = tree[parent].args[arg];
      // Not a divisor (expanded)
      if (idx >= 0) {
        toExpand.erase(toExpand.begin() + i);
        --i;
        continue;
      }

      const auto &divisor = getDivisor(divisors, idx);
      const auto &parentCare = tree[parent].care;
      const auto &sibling1 = getSiblingFunc(tree, divisors, parent, arg, 1);
      const auto &sibling2 = getSiblingFunc(tree, divisors, parent, arg, 2);

      const TruthTable care = parentCare & ~(sibling1 & sibling2);
      if (!mayImprove(divisor, care)) {
        toExpand.erase(toExpand.begin() + i);
        --i;
        continue;
      }

      const uint64_t uncovered = kitty::count_ones(care & ~divisor);
      if (minUncovered > uncovered) {
        pos = i;
        minUncovered = uncovered;
        oldFunc = divisor;
        oldCare = care;
      }
    }

    if (toExpand.empty()) {
      break;
    }

    Position position = toExpand[pos];
    toExpand.erase(toExpand.begin() + pos);

    MajNode expanded;
    expandNode(divisors, oldCare, expanded, position);

    const uint64_t oldCovered = kitty::count_ones(oldFunc & oldCare);
    const uint64_t nowCovered = kitty::count_ones(expanded.func & oldCare);
    if (oldCovered >= nowCovered) {
      continue;
    }

    expanded.care = oldCare;
    tree.emplace_back(expanded);
    tree[position.parent].args[position.arg] = tree.size() - 1;
    updateTree(tree, divisors, toExpand, position, oldFunc);

    if (!mayImprove(tree[0].func, tree[0].care)) {
      return true;
    }

    toExpand.push_back({tree.size() - 1, 0});
    toExpand.push_back({tree.size() - 1, 1});
    toExpand.push_back({tree.size() - 1, 2});
  }
  return false;
}

void DeMicheli::expandNode(const std::vector<TruthTable> &divisors,
                           const TruthTable &care,
                           MajNode &expanded,
                           Position position) const {

  uint64_t arg1 = heuristicArg1(divisors, care);
  uint64_t arg2 = heuristicArg2(arg1, divisors, care);
  uint64_t arg3 = heuristicArg3(arg1, arg2, divisors, care);

  expanded.args[0] = -arg1 - 1;
  expanded.args[1] = -arg2 - 1;
  expanded.args[2] = -arg3 - 1;
  expanded.func = kitty::ternary_majority(divisors[arg1],
                                          divisors[arg2],
                                          divisors[arg3]);
  expanded.position = position;
}

//===----------------------------------------------------------------------===//
// Tree updating
//===----------------------------------------------------------------------===//

void DeMicheli::updateTree(std::vector<MajNode> &tree,
                           const std::vector<TruthTable> &divisors,
                           std::vector<Position> &toExpand,
                           Position position,
                           const TruthTable &funcOld) const {

  const uint64_t parent = position.parent;
  const uint8_t arg = position.arg;

  const auto &sibling0 = getSiblingFunc(tree, divisors, parent, arg, 0);
  const auto &sibling1 = getSiblingFunc(tree, divisors, parent, arg, 1);
  const auto &sibling2 = getSiblingFunc(tree, divisors, parent, arg, 2);

  const TruthTable parentFunc = tree[parent].func;
  tree[parent].func = kitty::ternary_majority(sibling0, sibling1, sibling2);
  updateSibling(tree, divisors, toExpand, position,
                1, funcOld, sibling0, sibling2);
  updateSibling(tree, divisors, toExpand, position,
                2, funcOld, sibling0, sibling1);

  // update a grandparent
  uint64_t grandparentPos = tree[parent].position.parent;
  if (grandparentPos != OUTID) {
    Position pos = {grandparentPos, tree[parent].position.arg};
    updateTree(tree, divisors, toExpand, pos, parentFunc);
  }
}

void DeMicheli::updateSibling(std::vector<MajNode> &tree,
                              const std::vector<TruthTable> &divisors,
                              std::vector<Position> &toExpand,
                              Position pos,
                              uint8_t idx,
                              const TruthTable &funcOld,
                              const TruthTable &sibling0,
                              const TruthTable &sibling1) const {

  const uint64_t parent = pos.parent;
  const uint8_t arg = pos.arg;
  const uint8_t siblingId = (arg + idx) % 3;
  const int64_t siblingPos = tree[parent].args[siblingId];
  const auto &parentCare = tree[parent].care;
  const TruthTable careOld = parentCare & ~(funcOld & sibling1);
  const TruthTable careNew = parentCare & ~(sibling0 & sibling1);

  if (careOld != careNew) {
    // Not a divisor.
    if (siblingPos >= 0) {
      updateNode(tree, divisors, toExpand, siblingPos, careOld, careNew);
    } else {
      toExpand.push_back({parent, siblingId});
    }
  }
}

void DeMicheli::updateNode(std::vector<MajNode> &tree,
                           const std::vector<TruthTable> &divisors,
                           std::vector<Position> toExpand,
                           uint64_t siblingPos,
                           const TruthTable &careOld,
                           const TruthTable &careNew) const {

  const TruthTable &siblingFunc = tree[siblingPos].func;
  if (!mayImprove(siblingFunc, careOld) && mayImprove(siblingFunc, careNew)) {
    for (uint8_t i = 0; i < 3; ++i) {
      if (tree[siblingPos].args[i] < 0) {
        toExpand.push_back({siblingPos, i});
      }
    }
  }
  tree[siblingPos].care = careNew;

  for (uint8_t i = 0; i < 3; ++i) {
    const int64_t childPos = tree[siblingPos].args[i];
    if (childPos >= 0) {
      const auto &child1 = getSiblingFunc(tree, divisors, siblingPos, i, 1);
      const auto &child2 = getSiblingFunc(tree, divisors, siblingPos, i, 2);
      const TruthTable childOld = careOld & ~(child1 & child2);
      const TruthTable childNew = careNew & ~(child1 & child2);
      if (childOld != childNew) {
        updateNode(tree, divisors, toExpand, childPos, childOld, childNew);
      }
    }
  }
}

//===----------------------------------------------------------------------===//
// Enumeration of suitable top nodes
//===----------------------------------------------------------------------===//

void DeMicheli::createTopNodes(std::vector<MajNode> &topNodes,
                               std::vector<MajNode> &tree,
                               const std::vector<TruthTable> &divisors,
                               const std::vector<uint64_t> &nOnes,
                               const TruthTable &care) const {

  const uint64_t maxEl = *std::max_element(nOnes.begin(), nOnes.end());
  for (size_t i = 0; i < nOnes.size(); ++i) {
    if (nOnes[i] != maxEl) {
      continue;
    }
    if (selectOtherArgs(topNodes, divisors, i, care)) {
      tree = topNodes;
      return;
    }
  }
}

bool DeMicheli::selectOtherArgs(std::vector<MajNode> &topNodes,
                                const std::vector<TruthTable> &divisors,
                                size_t firstArg,
                                const TruthTable &care) const {

  const size_t invertedFirst = getInverted(firstArg);
  const size_t divisorsSize = divisors.size();
  uint64_t maxCount = 0;
  uint64_t sum[divisorsSize];
  const TruthTable &f1 = divisors[firstArg];
  const TruthTable &notF1 = divisors[invertedFirst];
  for (size_t i = 0; i < divisorsSize; ++i) {
    if ((i == firstArg) || (i == invertedFirst)) {
      sum[i] = 0;
      continue;
    }
    sum[i] = calculate(f1, notF1, divisors[i], care);
    if (sum[i] > maxCount) {
      maxCount = sum[i];
    }
  }
  for (size_t i = 0; i < divisorsSize; ++i) {
    if (sum[i] != maxCount) {
      continue;
    }
    if (selectLastArg(topNodes, divisors, firstArg, i, care)) {
      return true;
    }
  }
  return false;
}

bool DeMicheli::selectLastArg(std::vector<MajNode> &topNodes,
                              const std::vector<TruthTable> &divisors,
                              size_t firstArg,
                              size_t secondArg,
                              const TruthTable &care) const {

  const size_t invertedFirst = getInverted(firstArg);
  const size_t invertedSecond = getInverted(secondArg);
  const size_t divisorsSize = divisors.size();
  uint64_t maxCount = 0;
  uint64_t sum[divisorsSize];
  const TruthTable coveredOnes = divisors[firstArg] ^ divisors[secondArg];
  const TruthTable notCovered = divisors[invertedFirst] & 
                                divisors[invertedSecond];

  for (size_t i = 0; i < divisorsSize; ++i) {
    bool skip = (i == firstArg) || (i == invertedFirst);
    skip = skip || (i == secondArg) || (i == invertedSecond);
    if (skip) {
      sum[i] = 0;
      continue;
    }
    sum[i] = calculate(coveredOnes, notCovered, divisors[i], care);
    if (sum[i] > maxCount) {
      maxCount = sum[i];
    }
  }
  for (size_t i = 0; i < divisorsSize; ++i) {
    if (sum[i] != maxCount) {
      continue;
    }
    const TruthTable top = kitty::ternary_majority(divisors[firstArg],
                                                   divisors[secondArg],
                                                   divisors[i]);
    MajNode node;
    node.args[0] = -firstArg - 1;
    node.args[1] = -secondArg - 1;
    node.args[2] = -i - 1;
    if (kitty::is_const0(~top)) {
      topNodes.clear();
      topNodes.emplace_back(node);
      return true;
    }
    node.position.parent = OUTID;
    node.func = top;
    node.care = care;
    topNodes.emplace_back(node);
  }
  return false;
}

//===----------------------------------------------------------------------===//
// Build a subnet
//===----------------------------------------------------------------------===//

SubnetID DeMicheli::buildSubnet(const std::vector<MajNode> &tree,
                                const std::vector<TruthTable> &divisors,
                                bool simplest) const {

  const uint32_t nVars = divisors[0].num_vars();
  const size_t treeSize = tree.size();
  std::vector<size_t> idx;
  Builder builder;
  idx.reserve(nVars + tree.size());

  std::pair<size_t, bool> zero = {0, 0};

  for (size_t i = 0; i < nVars; ++i) {
    const size_t id = builder.addInput();
    idx.push_back(id);
  }

  auto it = tree.end();
  do {
    if (tree.empty()) {
      break;
    }
    --it;

    LinkList links(3);
    for (uint8_t i = 0; i < 3; ++i) {
      int64_t arg = it->args[i];
      links[i] = createLink(arg, nVars, treeSize, builder, idx, zero);
    }

    const size_t id = builder.addCell(CellSymbol::MAJ, links);
    idx.push_back(id);
  } while (it != tree.begin());

  if (simplest) {
    const uint64_t count = kitty::count_ones(divisors.back());
    const size_t id = count ? divisors.size() - 1 : divisors.size() - 2;
    const int64_t arg = -id - 1;
    Link link = createLink(arg, nVars, treeSize, builder, idx, zero);
    builder.addOutput(link);
  } else {
    builder.addOutput(Link(idx.back()));
  }

  return builder.make();
}

Link DeMicheli::createLink(int64_t arg,
                           uint32_t nVars,
                           size_t treeSize,
                           Builder &builder,
                           const std::vector<size_t> &idx,
                           std::pair<size_t, bool> &zero) const {

  // IN or NOT(IN).
  if (arg < -2) {
    arg = -arg - 1;
    const size_t in = arg / 2 - 1;
    return Link(idx[in], arg % 2);
  }
  // A MAJ node.
  if (arg > -1) {
    arg = nVars + treeSize - arg - 1;
    return Link(idx[arg]);
  }
  // Constants.
  if ((arg == -1) or (arg == -2)) {
    arg += 2;
    if (!zero.second) {
      zero.first = builder.addCell(CellSymbol::ZERO);
      zero.second = true;
    }
  }
  return Link(zero.first, !arg);
}

} // namespace eda::gate::optimizer2::resynthesis
