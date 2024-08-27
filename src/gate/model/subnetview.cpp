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

  uint32_t i = 0;
  for (auto it = parent.begin(); i < nIn; ++it, ++i) {
    const auto &cell = parent.getCell(*it);
    assert(cell.isIn());
    iomapping.inputs[i] = *it;
  }

  uint32_t j = 0;
  for (auto it = --parent.end(); j < nOut; --it, ++j) {
    const auto &cell = parent.getCell(*it);
    assert(cell.isOut());
    iomapping.outputs[nOut - j - 1] = *it;
  }
}

SubnetView::SubnetView(const SubnetBuilder &parent, const EntryID rootID):
    parent(parent) {
  // Find all reachable inputs for the given root cell.
  SubnetViewWalker walker(*this);

  iomapping.inputs.reserve(parent.getInNum());
  iomapping.outputs.push_back(rootID);

  walker.run([this](SubnetBuilder &parent,
                    const bool isIn,
                    const bool isOut,
                    const EntryID i) -> bool {
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
  assert(!cut.leafIDs.empty());

  iomapping.inputs.reserve(cut.leafIDs.size());
  for (const auto entryID : cut.leafIDs) {
    iomapping.inputs.push_back(entryID);
  }

  iomapping.outputs.push_back(cut.rootID);
}

SubnetView::SubnetView(const SubnetBuilder &parent,
                       const InOutMapping &iomapping):
    iomapping(iomapping), parent(parent) {
  assert(!iomapping.inputs.empty());
  assert(!iomapping.outputs.empty());
}

std::vector<SubnetView::TruthTable> SubnetView::evaluateTruthTables(
    const EntryIDList &entryIDs) const {
  std::vector<TruthTable> result(entryIDs.size());

  SubnetViewWalker walker(*this);
  const auto arity = getInNum();

  uint32_t nIn = 0;

  // Optimized calculator for views w/ a small number of inputs.
  if (arity <= 6) {
    walker.run([&nIn, arity](SubnetBuilder &parent,
                             const bool isIn,
                             const bool isOut,
                             const EntryID i) -> bool {
      const auto tt = utils::getTruthTable<utils::TT6>(
          parent, arity, i, isIn, nIn++);
      utils::setTruthTable<utils::TT6>(parent, i, tt);
      return true /* continue traversal */;
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
                                      const EntryID i) -> bool {
      const auto tt = utils::getTruthTable<TruthTable>(
          parent, arity, i, isIn, nIn++);
      tables.push_back(tt);
      utils::setTruthTable<TruthTable>(parent, i, tables.back());
      return true /* continue traversal */;
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
                              const EntryID i) -> bool {
    const auto &oldCell = parent.getCell(i);

    Subnet::Link newLink;
    if (isIn) {
      newLink = subnetBuilder.addInput();
    } else {
      assert(oldCell.getInNum() > 0);
      Subnet::LinkList newLinks(oldCell.getInNum());
      for (uint16_t j = 0; j < oldCell.getInNum(); ++j) {
        const auto &oldLink = parent.getLink(i, j);
        const auto newIdx = parent.getDataVal<EntryID>(oldLink.idx);
        const bool oldInv = oldLink.inv;
        newLinks[j] = Subnet::Link{newIdx, oldInv};
      }
      newLink = subnetBuilder.addCell(oldCell.getTypeID(), newLinks);
    }
    parent.setDataVal<EntryID>(i, newLink.idx);

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

static inline uint16_t defaultArityProvider(
    SubnetBuilder &builder, const EntryID entryID) {
  return builder.getCell(entryID).arity;
}

static inline Subnet::Link defaultLinkProvider(
    SubnetBuilder &builder, const EntryID entryID, const uint16_t linkIdx) {
  return builder.getLink(entryID, linkIdx);
}

SubnetViewWalker::SubnetViewWalker(const SubnetView &view):
    view(view),
    arityProvider(defaultArityProvider),
    linkProvider(defaultLinkProvider) {}

SubnetViewWalker::SubnetViewWalker(const SubnetView &view,
                                   const ArityProvider arityProvider,
                                   const LinkProvider linkProvider):
    view(view),
    arityProvider(arityProvider),
    linkProvider(linkProvider) {}

#define UTOPIA_ON_BACKWARD_DFS_POP(builder, isIn, isOut, cellID)\
  if (onBackwardDfsPop && !onBackwardDfsPop(builder, isIn, isOut, cellID))\
    goto ABORTED

#define UTOPIA_ON_BACKWARD_DFS_PUSH(builder, isIn, isOut, cellID)\
  if (onBackwardDfsPush && !onBackwardDfsPush(builder, isIn, isOut, cellID))\
    goto ABORTED

static bool traverseForward(SubnetBuilder &builder,
                            const InOutMapping &iomapping,
                            const SubnetViewWalker::ArityProvider arityProvider,
                            const SubnetViewWalker::LinkProvider linkProvider,
                            const SubnetViewWalker::Visitor onBackwardDfsPop,
                            const SubnetViewWalker::Visitor onBackwardDfsPush) {
  builder.startSession();

  uint32_t nPureOut{0};
  std::unordered_set<EntryID> inout;

  for (const auto inputID : iomapping.inputs) {
    builder.mark(inputID);
  } // for input

  std::stack<std::pair<EntryID /* entry */, uint16_t /* link */>> stack;
  for (const auto outputID : iomapping.outputs) {
    if (!builder.isMarked(outputID)) {
      UTOPIA_ON_BACKWARD_DFS_PUSH(builder, false, true, outputID);
      stack.push({outputID, 0});
    } else {
      inout.insert(outputID);
    }
  } // for output

  nPureOut = stack.size();

  for (const auto inputID : iomapping.inputs) {
    const auto isOut = (inout.find(inputID) != inout.end());
    UTOPIA_ON_BACKWARD_DFS_POP(builder, true, isOut, inputID);
  } // for input

  while (!stack.empty()) {
    auto &[i, j] = stack.top();
    const auto arity = arityProvider(builder, i);

    bool isFinished = true;
    for (; j < arity; ++j) {
      const auto link = linkProvider(builder, i, j);
      if (!builder.isMarked(link.idx)) {
        isFinished = false;
        UTOPIA_ON_BACKWARD_DFS_PUSH(builder, false, false, link.idx);
        stack.push({link.idx, 0});
        break;
      }
    } // for link

    if (isFinished) {
      const auto isOut = (stack.size() <= nPureOut);
      UTOPIA_ON_BACKWARD_DFS_POP(builder, false, isOut, i);
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

bool SubnetViewWalker::runForward(const Visitor onBackwardDfsPop,
                                  const Visitor onBackwardDfsPush,
                                  const bool saveEntries) {
  assert(onBackwardDfsPop || onBackwardDfsPush);
  auto &builder = const_cast<SubnetBuilder&>(view.getParent());

  if (entries && !onBackwardDfsPush) {
    return traverseForward(builder, *entries, onBackwardDfsPop);
  }
  if (!saveEntries) {
    return traverseForward(builder, view.getInOutMapping(),
        arityProvider, linkProvider, onBackwardDfsPop, onBackwardDfsPush);
  }

  entries = std::make_unique<Entries>();
  entries->reserve(builder.getCellNum());

  bool doPop{onBackwardDfsPop != nullptr};
  const Visitor onBackwardDfsPopEx =
      [this, &doPop, onBackwardDfsPop](SubnetBuilder &builder,
          const bool isIn, const bool isOut, const EntryID entryID) -> bool {
        doPop = doPop && onBackwardDfsPop(builder, isIn, isOut, entryID);
        entries->emplace_back(isIn, isOut, entryID);
        return true /* traverse all entries */;
      };

  bool doPush{onBackwardDfsPush != nullptr};
  const Visitor onBackwardDfsPushEx =
      [&doPush, onBackwardDfsPush](SubnetBuilder &builder,
          const bool isIn, const bool isOut, const EntryID entryID) -> bool {
        doPush = doPush && onBackwardDfsPush(builder, isIn, isOut, entryID);
        return true /* traverse all entries */;
      };

  return traverseForward(builder, view.getInOutMapping(),
      arityProvider, linkProvider, onBackwardDfsPopEx,
      (onBackwardDfsPush ? onBackwardDfsPushEx : nullptr));
}

bool SubnetViewWalker::runBackward(const Visitor visitor,
                                   const bool saveEntries) {
  auto &builder = const_cast<SubnetBuilder&>(view.getParent());

  if (!entries) {
    entries = std::make_unique<Entries>();
    entries->reserve(builder.getCellNum());

    const Visitor entrySaver = [this](SubnetBuilder &,
        const bool isIn, const bool isOut, const EntryID entryID) -> bool {
      entries->emplace_back(isIn, isOut, entryID);
      return true /* traverse all entries */;
    };

    // Do not return: fill the entries.
    traverseForward(builder, view.getInOutMapping(),
        arityProvider, linkProvider, entrySaver, nullptr);
  }

  return traverseBackward(builder, *entries, visitor);
}

} // namespace eda::gate::model
