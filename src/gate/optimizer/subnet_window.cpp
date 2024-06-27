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
// Subnet window
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
  iomapping.inputs.reserve(cut.entryIdxs.size());
  for (const auto entryID : cut.entryIdxs) {
    iomapping.inputs.push_back(entryID);
  }

  iomapping.outputs.push_back(cut.rootEntryIdx);
}

SubnetWindow::SubnetWindow(const model::SubnetBuilder &builder,
                           const InOutMapping &iomapping,
                           const TruthTable &care):
    iomapping(iomapping),
    care(care),
    builder(builder) {
  assert(!iomapping.inputs.empty());
  assert(!iomapping.outputs.empty());

  // TODO: Avoid copying.
  for (const auto inputID : iomapping.inputs) {
    in.insert(inputID);
  }
  for (const auto outputID : iomapping.outputs) {
    out.insert(outputID);
  }
}

SubnetWindow::TruthTable SubnetWindow::evaluateTruthTable() const {
  const SubnetWindowWalker walker(*this);
  const size_t arity = getInNum();

  size_t nIn = 0;

  // Optimized calculator for windows w/ a small number of inputs.
  if (arity <= 6) {
    walker.run([&nIn, arity](model::SubnetBuilder &builder, size_t i) {
      const auto in = nIn < arity;
      const auto tt = utils::getTruthTable<utils::TT6>(
          builder, arity, i, in, nIn++);
      utils::setTruthTable<utils::TT6>(builder, i, tt);
    });

    const auto tt = utils::getTruthTable<utils::TT6>(builder, iomapping.getOut(0));
    return utils::convertTruthTable<utils::TT6>(tt, arity);
  }

  std::vector<TruthTable> tables;
  tables.reserve(1024); // FIXME: if resize.

  walker.run([&tables, &nIn, arity](model::SubnetBuilder &builder, size_t i) {
    const auto in = nIn < arity;
    const auto tt = utils::getTruthTable<TruthTable>(
        builder, arity, i, in, nIn++);
    tables.push_back(tt);
    utils::setTruthTable<TruthTable>(builder, i, tables.back());
  });

  const auto tt = utils::getTruthTable<TruthTable>(builder, iomapping.getOut(0));
  return utils::convertTruthTable<TruthTable>(tt, arity);
}

// FIXME: Deprecated.
const model::Subnet &SubnetWindow::getSubnet() const {
  // TODO: Support multi-output windows.
  // TODO: Store the result of construction.
  assert(iomapping.getOutNum() == 1);

  const ConeBuilder coneBuilder(&builder);
  const auto cone = coneBuilder.getCone(iomapping.getOut(0), iomapping.inputs);

  return model::Subnet::get(cone.subnetID);
}

//===----------------------------------------------------------------------===//
// Subnet window walker
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
