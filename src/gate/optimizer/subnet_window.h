//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/cut_extractor.h"

#include <kitty/kitty.hpp>

#include <cstddef>
#include <functional>
#include <unordered_set>
#include <vector>

namespace eda::gate::optimizer {

//===----------------------------------------------------------------------===//
// Subnet Window
//===----------------------------------------------------------------------===//

/**
 * @brief Functionally closed subnet fragment.
 */
class SubnetWindow final {
public:
  using TruthTable = kitty::dynamic_truth_table;

  SubnetWindow(const model::SubnetBuilder &builder,
               const CutExtractor::Cut &cut,
               const TruthTable &care);

  SubnetWindow(const model::SubnetBuilder &builder,
               const CutExtractor::Cut &cut):
      SubnetWindow(builder, cut, TruthTable{}) {}

  SubnetWindow(const model::SubnetBuilder &builder,
               const std::vector<size_t> &inputs,
               const std::vector<size_t> &outputs,
               const TruthTable &care);

  SubnetWindow(const model::SubnetBuilder &builder,
               const std::vector<size_t> &inputs,
               const std::vector<size_t> &outputs):
      SubnetWindow(builder, inputs, outputs, TruthTable{}) {}

  size_t getInNum() const { return inputs.size(); }
  size_t getOutNum() const { return outputs.size(); }

  size_t getIn(const size_t i) const { return inputs[i]; }
  size_t getOut(const size_t i) const { return outputs[i]; }

  bool hasIn(const size_t i) const { return in.find(i) != in.end(); }
  bool hasOut(const size_t i) const { return out.find(i) != out.end(); }

  const std::vector<size_t> &getInputs() const { return inputs; }
  const std::vector<size_t> &getOutputs() const { return outputs; }

  const TruthTable &getCare() const { return care; }
  void setCare(const TruthTable &care) { this->care = care; }

  TruthTable evaluateTruthTable() const;

  const model::SubnetBuilder &getBuilder() const { return builder; }

  // FIXME: Deprecated.
  const model::Subnet &getSubnet() const;

  // FIXME: Deprecated.
  std::unordered_map<size_t, size_t> getInOutMapping(const model::Subnet &subnet) const;

private:
  /// Ordered inputs.
  std::vector<size_t> inputs;
  /// Ordered outputs.
  std::vector<size_t> outputs;

  /// Common care specification for all outputs.
  TruthTable care;

  /// FIXME: Fast membership checking for inputs.
  std::unordered_set<size_t> in;
  /// FIXME: Fast membership checking for outputs.
  std::unordered_set<size_t> out;

  /// Parent subnet builder.
  const model::SubnetBuilder &builder;
};

//===----------------------------------------------------------------------===//
// Subnet Window Walker
//===----------------------------------------------------------------------===//

/**
 * @brief DFS subnet window walker.
 */
class SubnetWindowWalker final {
public:
  using Visitor = std::function<
    void(model::SubnetBuilder &builder, const size_t entryID)>;

  SubnetWindowWalker(const SubnetWindow &window):
    window(window) {}

  /// Visits the cells of the subnet window in topological order.
  void run(const Visitor visitor) const;

private:
  const SubnetWindow &window;
};

} // namespace eda::gate::optimizer
