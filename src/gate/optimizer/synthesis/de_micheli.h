//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/celltype.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/synthesizer.h"
#include "util/kitty_utils.h"

#include <kitty/constructors.hpp>
#include <kitty/kitty.hpp>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace eda::gate::optimizer::synthesis {

/// The positon of a node related to a parent.
struct Position {
  uint64_t parent : 62;
  uint64_t arg : 2;
};

/// A MAJ node of a tree.
struct MajNode {
  int64_t args[3];
  Position position;
  kitty::dynamic_truth_table func;
  kitty::dynamic_truth_table care;
};

/**
 * @brief Implements a De Micheli approach of synthesis based on MAJ gates.
 * 
 * The algorithm is based on the article "Logic Resynthesis of
 * Majority-Based Logic using Top-Down Decomposition" by Siang-Yun Lee,
 * Heinz Riener and Giovanni De Micheli.
 * Published in: 2021 24th International Symposium on
 * Design and Diagnostics of Electronic Circuits & Systems (DDECS).
 * DOI: 10.1109/DDECS52668.2021.9417058
 * Publisher: IEEE
 */
class DMSynthesizer : public TruthTableSynthesizer {

public:

  using Builder      = eda::gate::model::SubnetBuilder;
  using CellSymbol   = eda::gate::model::CellSymbol;
  using Link         = eda::gate::model::Subnet::Link;
  using LinkList     = eda::gate::model::Subnet::LinkList;
  using SubnetObject = eda::gate::model::SubnetObject;
  using TruthTable   = utils::TruthTable;

  static constexpr size_t BOUND = 2000;
  static constexpr size_t OUTID = 4611686018427387903;

  DMSynthesizer() {}

  using Synthesizer::synthesize;

  SubnetObject synthesize(const TruthTable &func, const TruthTable &care,
                          uint16_t maxArity = -1) const override;

private:

  uint64_t heuristicArg1(const std::vector<TruthTable> &divisors,
                         const TruthTable &care) const;

  uint64_t heuristicArg2(uint64_t arg1,
                         const std::vector<TruthTable> &divisors,
                         const TruthTable &care) const;                                    

  uint64_t heuristicArg3(uint64_t arg1,
                         uint64_t arg2,
                         const std::vector<TruthTable> &divisors,
                         const TruthTable &care) const;

  uint64_t calculate(const TruthTable &table1,
                     const TruthTable &table2,
                     const TruthTable &candidate,
                     const TruthTable &care) const;

  bool createDivisors(const TruthTable &func,
                      std::vector<TruthTable> &divisors,
                      std::vector<uint64_t> &nOnes) const;

  void run(std::vector<MajNode> &topNodes,
           std::vector<MajNode> &tree,
           const std::vector<TruthTable> &divisors) const;

  bool isSuccessBuild(std::vector<MajNode> &tree,
                      std::vector<Position> &toExpand,
                      const std::vector<TruthTable> &divisors) const;

  void expandNode(const std::vector<TruthTable> &divisors,
                  const TruthTable &care,
                  MajNode &expanded,
                  Position position) const;

  void updateTree(std::vector<MajNode> &tree,
                  const std::vector<TruthTable> &divisors,
                  std::vector<Position> &toExpand,
                  Position position,
                  const TruthTable &funcOld) const;

  void updateSibling(std::vector<MajNode> &tree,
                     const std::vector<TruthTable> &divisors,
                     std::vector<Position> &toExpand,
                     Position pos, 
                     uint8_t idx,
                     const TruthTable &funcOld,
                     const TruthTable &sibling0,
                     const TruthTable &sibling1) const;

  void updateNode(std::vector<MajNode> &tree, 
                  const std::vector<TruthTable> &divisors,
                  std::vector<Position> toExpand,
                  uint64_t siblingPos,
                  const TruthTable &careOld,
                  const TruthTable &careNew) const;

  void createTopNodes(std::vector<MajNode> &topNodes,
                      std::vector<MajNode> &tree,
                      const std::vector<TruthTable> &divisors,
                      const std::vector<uint64_t> &nOnes,
                      const TruthTable &care) const;

  bool selectOtherArgs(std::vector<MajNode> &topNodes,
                       const std::vector<TruthTable> &divisors,
                       size_t firstArg,
                       const TruthTable &care) const;

  bool selectLastArg(std::vector<MajNode> &topNodes,
                     const std::vector<TruthTable> &divisors,
                     size_t firstArg,
                     size_t secondArg,
                     const TruthTable &care) const;

  SubnetObject buildSubnet(const std::vector<MajNode> &tree,
                           const std::vector<TruthTable> &divisors,
                           bool simplest = false) const;

  Link createLink(int64_t arg,
                  uint32_t nVars,
                  size_t treeSize,
                  Builder &builder,
                  const std::vector<size_t> &idx,
                  std::pair<size_t, bool> &zero) const;

  size_t getInverted(size_t pos) const {
    return (pos % 2) ? pos - 1 : pos + 1;
  }

  const TruthTable &getDivisor(const std::vector<TruthTable> &divisors,
                               int64_t idx) const {

    assert(idx < 0 && "Invalid index of a divisor!");
    return divisors[-idx - 1];
  }

  const TruthTable &getFunction(const std::vector<MajNode> &tree,
                                const std::vector<TruthTable> &divisors,
                                int64_t pos) const {
    if (pos < 0) {
      return getDivisor(divisors, pos);
    }
    return tree[pos].func;
  }

  const TruthTable &getSiblingFunc(const std::vector<MajNode> &tree,
                                   const std::vector<TruthTable> &divisors,
                                   uint64_t nodeNum,
                                   uint8_t arg,
                                   uint8_t siblingNum) const {

    const uint8_t argIndex = (arg + siblingNum) % 3;
    const int64_t sibIndex = tree[nodeNum].args[argIndex];
    return getFunction(tree, divisors, sibIndex);
  }

  bool mayImprove(const TruthTable &divisor, const TruthTable &care) const {
    return !utils::isZero(~divisor & care);
  }
};

} // namespace eda::gate::optimizer::synthesis
