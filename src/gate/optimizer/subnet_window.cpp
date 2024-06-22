//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/cone_builder.h"
#include "gate/optimizer/subnet_window.h"

#include <cassert>

namespace eda::gate::optimizer {

SubnetWindow::SubnetWindow(const model::SubnetBuilder &builder,
                           const CutExtractor::Cut &cut,
                           const TruthTable &care):
    care(care),
    in(cut.entryIdxs),
    out({cut.rootEntryIdx}),
    builder(builder) {
  assert(!cut.entryIdxs.empty());

  // TODO: Avoid copying.
  inputs.reserve(cut.entryIdxs.size());
  for (const auto entryID : cut.entryIdxs) {
    inputs.push_back(entryID);
  }

  outputs.push_back(cut.rootEntryIdx);
}

SubnetWindow::SubnetWindow(const model::SubnetBuilder &builder,
                           const std::vector<size_t> &inputs,
                           const std::vector<size_t> &outputs,
                           const TruthTable &care):
    inputs(inputs),
    outputs(outputs),
    care(care),
    builder(builder) {
  assert(!inputs.empty());
  assert(!outputs.empty());

  // TODO: Avoid copying.
  for (const auto entryID : inputs) {
    in.insert(entryID);
  }
  for (const auto entryID : outputs) {
    out.insert(entryID);
  }
}

SubnetWindow::TruthTable SubnetWindow::evaluateTruthTable() const {
  // FIXME: Calculate the truth table w/o constructing the subnet.
  return model::evaluateSingleOut(getSubnet());
}

// FIXME: Deprecated.
const model::Subnet &SubnetWindow::getSubnet() const {
  // TODO: Support multi-output windows.
  // TODO: Store the result of construction.
  assert(outputs.size() == 1);

  const ConeBuilder coneBuilder(&builder);
  const auto cone = coneBuilder.getCone(outputs[0], inputs);

  return model::Subnet::get(cone.subnetID);
}

// FIXME: Deprecated.
std::unordered_map<size_t, size_t> SubnetWindow::getInOutMapping(
    const model::Subnet &subnet) const {
  assert(subnet.getInNum() == inputs.size());
  assert(subnet.getOutNum() == outputs.size());

  std::unordered_map<size_t, size_t> mapping;
  for (size_t i = 0; i < inputs.size(); ++i) {
    mapping[subnet.getInIdx(i)] = inputs[i];
  }
  for (size_t i = 0; i < outputs.size(); ++i) {
    mapping[subnet.getOutIdx(i)] = outputs[i];
  }
  return mapping;
}

} // namespace eda::gate::optimizer
