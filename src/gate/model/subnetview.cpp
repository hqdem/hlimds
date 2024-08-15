//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnetview.h"

#include <cassert>
#include <unordered_set>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet View
//===----------------------------------------------------------------------===//

SubnetView::SubnetView(const SubnetBuilder &parent):
    parent(parent) {
  const auto nIn = parent.getInNum();
  iomapping.inputs.resize(nIn);

  const auto nOut = parent.getOutNum();
  iomapping.outputs.resize(nOut);

  size_t i = 0;
  for (auto it = parent.begin(); i < nIn; ++it, ++i) {
    const auto &cell = parent.getCell(*it);
    assert(cell.isIn());
    iomapping.inputs[i] = *it;
  }

  size_t j = 0;
  for (auto it = --parent.end(); j < nOut; --it, ++j) {
    const auto &cell = parent.getCell(*it);
    assert(cell.isOut());
    iomapping.outputs[nOut - j - 1] = *it;
  }
}

SubnetView::SubnetView(const SubnetBuilder &parent, const size_t rootID):
    parent(parent) {
  // Find all reachable inputs for the given root cell.
  SubnetViewWalker walker(*this);

  iomapping.inputs.reserve(parent.getInNum());
  iomapping.outputs.push_back(rootID);

  walker.run([this](SubnetBuilder &parent,
                    const bool isIn,
                    const bool isOut,
                    const size_t i) -> bool {
    const auto &cell = parent.getCell(i);
    if (cell.isIn() || cell.isZero() || cell.isOne()) {
      iomapping.inputs.push_back(i);
      return true;
    }
    // Stop traversal when root is visited.
    return i != iomapping.getOut(0);
  });
}

SubnetView::SubnetView(const SubnetBuilder &parent, const Cut &cut):
    parent(parent) {
  assert(!cut.entryIdxs.empty());

  iomapping.inputs.reserve(cut.entryIdxs.size());
  for (const auto entryID : cut.entryIdxs) {
    iomapping.inputs.push_back(entryID);
  }

  iomapping.outputs.push_back(cut.rootEntryIdx);
}

SubnetView::SubnetView(const SubnetBuilder &parent,
                       const InOutMapping &iomapping):
    iomapping(iomapping), parent(parent) {
  assert(!iomapping.inputs.empty());
  assert(!iomapping.outputs.empty());
}

std::vector<SubnetView::TruthTable> SubnetView::evaluateTruthTables(
    const std::vector<size_t> &entryIDs) const {
  std::vector<TruthTable> result(entryIDs.size());

  SubnetViewWalker walker(*this);
  const size_t arity = getInNum();

  size_t nIn = 0;

  // Optimized calculator for views w/ a small number of inputs.
  if (arity <= 6) {
    walker.run([&nIn, arity](SubnetBuilder &parent,
                             const bool isIn,
                             const bool isOut,
                             const size_t i) -> bool {
      const auto tt = utils::getTruthTable<utils::TT6>(
          parent, arity, i, isIn, nIn++);
      utils::setTruthTable<utils::TT6>(parent, i, tt);
      return true; // Continue traversal.
    });

    for (size_t i = 0; i < entryIDs.size(); ++i) {
      const auto tt = utils::getTruthTable<utils::TT6>(parent, entryIDs[i]);
      result[i] = utils::convertTruthTable<utils::TT6>(tt, arity);
    }
  } else {
    std::vector<TruthTable> tables;
    tables.reserve(parent.getCellNum());

    walker.run([&tables, &nIn, arity](SubnetBuilder &parent,
                                      const bool isIn,
                                      const bool isOut,
                                      const size_t i) -> bool {
      const auto tt = utils::getTruthTable<TruthTable>(
          parent, arity, i, isIn, nIn++);
      tables.push_back(tt);
      utils::setTruthTable<TruthTable>(parent, i, tables.back());
      return true; // Continue traversal.
    });

    for (size_t i = 0; i < entryIDs.size(); ++i) {
      const auto tt = utils::getTruthTable<utils::TTn>(parent, entryIDs[i]);
      result[i] = utils::convertTruthTable<utils::TTn>(tt, arity);
    }
  }

  return result;
}

SubnetObject &SubnetView::getSubnet() {
  if (!subnet.isNull()) {
    return subnet;
  }

  SubnetViewWalker walker(*this);
  auto &subnetBuilder = subnet.builder();

  walker.run([&subnetBuilder](SubnetBuilder &parent,
                              const bool isIn,
                              const bool isOut,
                              const size_t i) -> bool {
    const auto &oldCell = parent.getCell(i);

    Subnet::Link newLink;
    if (isIn) {
      newLink = subnetBuilder.addInput();
    } else {
      assert(oldCell.getInNum() > 0);
      Subnet::LinkList newLinks(oldCell.getInNum());
      for (size_t j = 0; j < oldCell.getInNum(); ++j) {
        const auto &oldLink = parent.getLink(i, j);
        const auto newIdx = parent.getDataVal<uint32_t>(oldLink.idx);
        const bool oldInv = oldLink.inv;
        newLinks[j] = Subnet::Link{newIdx, oldInv};
      }
      newLink = subnetBuilder.addCell(oldCell.getTypeID(), newLinks);
    }
    parent.setDataVal<uint32_t>(i, newLink.idx);

    if (isOut && !oldCell.isOut()) {
      subnetBuilder.addOutput(newLink);
    }

    return true;
  });

  assert(!subnet.isNull());
  return subnet;
}

//===----------------------------------------------------------------------===//
// Subnet View Walker
//===----------------------------------------------------------------------===//

static bool traverseForward(SubnetBuilder &builder,
                            const InOutMapping &iomapping,
                            const SubnetViewWalker::Visitor visitor) {
  builder.startSession();

  std::unordered_set<size_t> inout;

  for (const auto inputID : iomapping.inputs) {
    builder.mark(inputID);
  } // for input

  std::stack<std::pair<size_t, size_t>> stack;
  for (const auto outputID : iomapping.outputs) {
    if (!builder.isMarked(outputID)) {
      stack.push({outputID, 0});
    } else {
      inout.insert(outputID);
    }
  } // for output

  const auto nOut = stack.size();

  for (const auto inputID : iomapping.inputs) {
    const auto isIn = true;
    const auto isOut = (inout.find(inputID) != inout.end());
    // Call the visitor.
    if (!visitor(builder, isIn, isOut, inputID)) {
      goto ABORTED;
    }
  } // for input

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
      const auto isIn = false;
      const auto isOut = stack.size() <= nOut;
      // Call the visitor.
      if (!visitor(builder, isIn, isOut, i)) {
        goto ABORTED;
      }
      builder.mark(i);
      stack.pop();
    }
  } // while stack

  builder.endSession();
  return true;

ABORTED:
  builder.endSession();
  return false;
}

static inline bool traverseForward(SubnetBuilder &builder,
                                   const SubnetViewWalker::Entries &entries,
                                   const SubnetViewWalker::Visitor visitor) {
  for (auto i = entries.begin(); i != entries.end(); ++i) {
    if (!visitor(builder, i->isIn, i->isOut, i->entryID)) return false;
  }
  return true;
}

static inline bool traverseBackward(SubnetBuilder &builder,
                                    const SubnetViewWalker::Entries &entries,
                                    const SubnetViewWalker::Visitor visitor) {
  for (auto i = entries.rbegin(); i != entries.rend(); ++i) {
    if (!visitor(builder, i->isIn, i->isOut, i->entryID)) return false;
  }
  return true;
}

bool SubnetViewWalker::run(const Visitor visitor,
                           const Direction direction,
                           const bool saveEntries) {
  SubnetBuilder &builder = const_cast<SubnetBuilder&>(view.getParent());

  if (direction == FORWARD) {
    if (entries)
      return traverseForward(builder, *entries, visitor);
    if (!saveEntries)
      return traverseForward(builder, view.getInOutMapping(), visitor);

    bool status{true};
    entries = std::make_unique<Entries>();
    entries->reserve(builder.getCellNum());

    const Visitor visitorAndFiller = [this, &status, visitor](SubnetBuilder &builder,
        const bool isIn, const bool isOut, const size_t entryID) -> bool {
      status = status && visitor(builder, isIn, isOut, entryID);
      entries->emplace_back(isIn, isOut, entryID);
      return true /* traverse all entries */;
    };

    return traverseForward(builder, view.getInOutMapping(), visitorAndFiller);
  }

  // Traverse backward.
  if (!entries) {
    entries = std::make_unique<Entries>();
    entries->reserve(builder.getCellNum());

    const Visitor filler = [this](SubnetBuilder &,
        const bool isIn, const bool isOut, const size_t entryID) -> bool {
      entries->emplace_back(isIn, isOut, entryID);
      return true /* traverse all entries */;
    };

    // Do not return: fill the entries.
    traverseForward(builder, view.getInOutMapping(), filler);
  }

  return traverseBackward(builder, *entries, visitor);
}

} // namespace eda::gate::model
