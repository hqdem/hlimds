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
#include "util/truth_table.h"

#include <cassert>
#include <utility>

namespace eda::gate::optimizer {

//===----------------------------------------------------------------------===//
// Subnet Window
//===----------------------------------------------------------------------===//

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
  for (const auto inputID : inputs) {
    in.insert(inputID);
  }
  for (const auto outputID : outputs) {
    out.insert(outputID);
  }
}

SubnetWindow::TruthTable SubnetWindow::evaluateTruthTable() const {
  const SubnetWindowWalker walker(*this);
  const size_t arity = getInNum();

  size_t nIn = 0;

  // Optimized calculator for windows w/ a small number of inputs.
  if (inputs.size() <= 6) {
    walker.run([&nIn, arity](model::SubnetBuilder &builder, size_t i) {
      const auto in = nIn < arity;
      const auto tt = utils::getTruthTable<utils::TT6>(
          builder, arity, i, in, nIn++);
      utils::setTruthTable<utils::TT6>(builder, i, tt);
    });

    const auto tt = utils::getTruthTable<utils::TT6>(builder, outputs[0]);
    return utils::convertTruthTable<utils::TT6>(tt, arity);
  }

  std::vector<TruthTable> tables;
  tables.reserve(1024); // FIXME: if resize.

  walker.run([&tables, &nIn, arity](model::SubnetBuilder &builder, size_t i) {
    const auto in = nIn < arity;
    const auto tt = utils::getTruthTable<TruthTable>(
        builder, arity, i, in, nIn++);
    tables.push_back(tt);
    utils::setTruthTable<TruthTable>(builder, i, tables[tables.size() - 1]);
  });

  const auto tt = utils::getTruthTable<TruthTable>(builder, outputs[0]);
  return utils::convertTruthTable<TruthTable>(tt, arity);
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

//===----------------------------------------------------------------------===//
// Subnet Window Walker
//===----------------------------------------------------------------------===//

void SubnetWindowWalker::run(const Visitor visitor) const {
  model::SubnetBuilder &builder =
      const_cast<model::SubnetBuilder&>(window.getBuilder());

  builder.startSession();

  for (const auto inputID : window.getInputs()) {
    visitor(builder, inputID);
    builder.mark(inputID);
  }

  std::stack<std::pair<size_t, size_t>> stack;
  for (const auto outputID : window.getOutputs()) {
    if (!builder.isMarked(outputID)) {
      stack.push({outputID, 0});
    }
  }

  while (!stack.empty()) {
    auto &[i, j] = stack.top();
    const auto &cell = builder.getCell(i);

    bool isFinished = true;
    for (; j < cell.arity; ++j) {
      const auto link = builder.getLink(i, j);
      if (!builder.isMarked(link.idx)) {
        isFinished = false;
        stack.push({link.idx, 0});
        break;
      }
    } // for link

    if (isFinished) {
      visitor(builder, i);
      builder.mark(i);
      stack.pop();
    }
  } // while stack

  builder.endSession();
}

} // namespace eda::gate::optimizer
