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

using TT = SubnetWindow::TruthTable;
using SB = model::SubnetBuilder;
using Link = model::Subnet::Link;
using Cell = model::Subnet::Cell;

static inline const TT &getTT(const SB &builder, const size_t i) {
  assert(builder.isMarked(i));
  return *builder.getData<TT>(i);
}

static inline TT getTT(const SB &builder, const Link &link) {
  const auto &tt = getTT(builder, link.idx);
  return link.inv ? ~tt : tt;
}

static inline TT getTT(const SB &builder, const size_t i, const size_t j) {
  return getTT(builder, builder.getLink(i, j));
}

static inline TT getInTT(const size_t nIn, const size_t i) {
  auto tt = kitty::create<TT>(nIn);
  kitty::create_nth_var(tt, i);
  return tt;
}

static inline TT getZeroTT(const size_t nIn) {
  auto tt = kitty::create<TT>(nIn);
  kitty::clear(tt);
  return tt;
}

static inline TT getOneTT(const size_t nIn) {
  return ~getZeroTT(nIn);
}

static inline TT getBufTT(const SB &builder, const Cell &cell) {
  return getTT(builder, cell.link[0]);
}

static inline TT getAndTT(const SB &builder, const Cell &cell, const size_t i) {
  auto tt = getTT(builder, cell.link[0]);
  for (size_t j = 1; j < cell.arity; ++j) {
    tt &= getTT(builder, i, j);
  }
  return tt;
}

static inline TT getOrTT(const SB &builder, const Cell &cell, const size_t i) {
  auto tt = getTT(builder, cell.link[0]);
  for (size_t j = 1; j < cell.arity; ++j) {
    tt |= getTT(builder, i, j);
  }
  return tt;
}

static inline TT getXorTT(const SB &builder, const Cell &cell, const size_t i) {
  auto tt = getTT(builder, cell.link[0]);
  for (size_t j = 1; j < cell.arity; ++j) {
    tt ^= getTT(builder, i, j);
  }
  return tt;
}

static inline TT getMajTT(const SB &builder, const Cell &cell, const size_t i) {
  auto tt = getTT(builder, cell.link[0]);
  kitty::clear(tt);

  std::vector<TT> args(cell.arity);
  for (size_t j = 0; j < cell.arity; ++j) {
    args[j] = getTT(builder, i, j);
  }

  const auto threshold = cell.arity >> 1;
  for (size_t k = 0; k < tt.num_bits(); ++k) {
    auto count = 0;
    for (size_t j = 0; j < cell.arity; ++j) {
      if (get_bit(args[j], k)) count++;
    }
    if (count > threshold) {
      set_bit(tt, k);
    }
  }

  return tt;
}

SubnetWindow::TruthTable SubnetWindow::evaluateTruthTable() const {
  std::vector<TruthTable> tables;
  tables.reserve(1024); // FIXME:

  size_t nIn = 0;

  const SubnetWindowWalker walker(*this);
  walker.run([&tables, &nIn, this](model::SubnetBuilder &builder, const size_t entryID) {
    const auto &cell = builder.getCell(entryID);
    TruthTable tt;

    if (nIn < getInNum())   { tt = getInTT  (getInNum(), nIn++);        }
    else if (cell.isZero()) { tt = getZeroTT(getInNum());               }
    else if (cell.isOne())  { tt = getOneTT (getInNum());               }
    else if (cell.isOut())  { tt = getBufTT (builder, cell);            }
    else if (cell.isBuf())  { tt = getBufTT (builder, cell);            }
    else if (cell.isAnd())  { tt = getAndTT (builder, cell, entryID);   }
    else if (cell.isOr())   { tt = getOrTT  (builder, cell, entryID);   }
    else if (cell.isXor())  { tt = getXorTT (builder, cell, entryID);   }
    else if (cell.isMaj())  { tt = getMajTT (builder, cell, entryID);   }
    else                    { assert(false && "Unsupported operation"); }

    tables.push_back(tt);
    builder.setData(entryID, &tables[tables.size() - 1]); // FIXME: if resize?
  });

  return getTT(builder, outputs[0]);
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
