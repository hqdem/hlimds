//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnetview.h"
#include "gate/model/subnet.h"

#include <cassert>
#include <unordered_set>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet View
//===----------------------------------------------------------------------===//

SubnetView::SubnetView(const std::shared_ptr<SubnetBuilder> &builder):
    parent(builder) {
  const auto nIn = builder->getInNum();
  iomapping.inputs.resize(nIn);

  const auto nOut = builder->getOutNum();
  iomapping.outputs.resize(nOut);

  SubnetSz i = 0;
  for (auto it = builder->begin(); i < nIn; ++it, ++i) {
    const auto &cell = builder->getCell(*it);
    assert(cell.isIn());
    iomapping.inputs[i] = Link(*it);
  }

  SubnetSz j = 0;
  for (auto it = --builder->end(); j < nOut; --it, ++j) {
    const auto &cell = builder->getCell(*it);
    assert(cell.isOut());
    iomapping.outputs[nOut - j - 1] = Link(*it);
  }
}

SubnetView::SubnetView(const std::shared_ptr<SubnetBuilder> &builder,
                       const EntryID rootID):
    parent(builder) {
  // Find all reachable inputs for the given root cell.
  SubnetViewWalker walker(*this);

  iomapping.inputs.reserve(builder->getInNum());
  iomapping.outputs.push_back(Link(rootID));

  walker.run([this](SubnetBuilder &parent,
                    const bool isIn,
                    const bool isOut,
                    const EntryID i) -> bool {
    const auto &cell = parent.getCell(i);
    if (cell.isIn() || cell.isZero() || cell.isOne()) {
      iomapping.inputs.push_back(Link(i));
      return true;
    }

    // Stop traversal when root is visited.
    return i != iomapping.getOut(0).idx;
  });
}

SubnetView::SubnetView(const std::shared_ptr<SubnetBuilder> &builder,
                       const Cut &cut):
    parent(builder) {
  assert(!cut.leafIDs.empty());

  iomapping.inputs.reserve(cut.leafIDs.size());
  for (const auto entryID : cut.leafIDs) {
    iomapping.inputs.push_back(Link(entryID));
  }

  iomapping.outputs.push_back(Link(cut.rootID));
}

SubnetView::SubnetView(const std::shared_ptr<SubnetBuilder> &builder,
                       const InOutMapping &iomapping):
    iomapping(iomapping), parent(builder) {
  assert(!iomapping.inputs.empty());
  assert(!iomapping.outputs.empty());
}

std::vector<SubnetView::TruthTable> SubnetView::evaluateTruthTables(
    const InOutMapping::LinkList &entryLinks) const {
  std::vector<TruthTable> result(entryLinks.size());

  SubnetViewWalker walker(*this);
  const auto arity = getInNum();

  SubnetSz nIn = 0;

  // Optimized calculator for views w/ a small number of inputs.
  if (arity <= 6) {
    walker.run([&nIn, arity](SubnetBuilder &parent,
                             const bool isIn,
                             const bool isOut,
                             const EntryID i) -> bool {
      const auto tt = util::getTruthTable<util::TT6>(
          parent, arity, i, isIn, nIn++);
      util::setTruthTable<util::TT6>(parent, i, tt);
      return true /* continue traversal */;
    });

    for (size_t i = 0; i < entryLinks.size(); ++i) {
      const auto tt = util::getTruthTable<util::TT6>(parent.builder(),
                                                     entryLinks[i].idx);
      result[i] = util::convertTruthTable<util::TT6>(tt, arity);
    }
  } else {
    std::vector<TruthTable> tables;
    tables.reserve(parent.builder().getCellNum());

    walker.run([&tables, &nIn, arity](SubnetBuilder &parent,
                                      const bool isIn,
                                      const bool isOut,
                                      const EntryID i) -> bool {
      const auto tt = util::getTruthTable<TruthTable>(
          parent, arity, i, isIn, nIn++);
      tables.push_back(tt);
      util::setTruthTable<TruthTable>(parent, i, tables.back());
      return true /* continue traversal */;
    });

    for (size_t i = 0; i < entryLinks.size(); ++i) {
      const auto tt = util::getTruthTable<util::TTn>(parent.builder(),
                                                     entryLinks[i].idx);
      result[i] = util::convertTruthTable<util::TTn>(tt, arity);
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
      for (SubnetSz j = 0; j < oldCell.getInNum(); ++j) {
        const auto &oldLink = parent.getLink(i, j);
        const auto newIdx = parent.getDataVal<EntryID>(oldLink.idx);
        const bool oldInv = oldLink.inv;
        newLinks[j] = Subnet::Link{newIdx, oldInv};
      }
      newLink = subnetBuilder.addCell(oldCell.getTypeID(), newLinks);
    }
    parent.setDataVal<EntryID>(i, newLink.idx);

    const auto notAddedAsOutput = isIn || !oldCell.isOut();
    if (isOut && notAddedAsOutput) {
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

#define UTOPIA_ON_BACKWARD_DFS_POP(builder, isIn, isOut, cellID)\
  if (onBackwardDfsPop && !isAborted &&\
    !onBackwardDfsPop(builder, isIn, isOut, cellID)) {\
    isAborted = true;\
  }

#define UTOPIA_ON_BACKWARD_DFS_PUSH(builder, isIn, isOut, cellID)\
  if (onBackwardDfsPush && !isAborted &&\
    !onBackwardDfsPush(builder, isIn, isOut, cellID)) {\
    isAborted = true;\
  }

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

static bool traverseForward(SubnetBuilder &builder,
                            const InOutMapping &iomapping,
                            const SubnetViewWalker::ArityProvider arityProvider,
                            const SubnetViewWalker::LinkProvider linkProvider,
                            const SubnetViewWalker::Visitor onBackwardDfsPop,
                            const SubnetViewWalker::Visitor onBackwardDfsPush) {
  SubnetView view(std::make_shared<SubnetBuilder>(builder), iomapping);
  SubnetViewWalker walker(view, arityProvider, linkProvider);
  return walker.runForward(onBackwardDfsPop, onBackwardDfsPush);
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
  auto &builder = const_cast<SubnetBuilder&>(view.getParent().builder());

  if (entries && !onBackwardDfsPush) {
    return traverseForward(builder, *entries, onBackwardDfsPop);
  }

  Visitor onBackwardDfsPopEx;
  Visitor onBackwardDfsPushEx;

  if (saveEntries) {
    entries = std::make_unique<Entries>();
    entries->reserve(builder.getCellNum());

    bool doPop{onBackwardDfsPop != nullptr};
    onBackwardDfsPopEx =
        [this, &doPop, onBackwardDfsPop](SubnetBuilder &builder,
            const bool isIn, const bool isOut, const EntryID entryID) -> bool {
          doPop = doPop && onBackwardDfsPop(builder, isIn, isOut, entryID);
          entries->emplace_back(isIn, isOut, entryID);
          return true /* traverse all entries */;
        };

    bool doPush{onBackwardDfsPush != nullptr};
    onBackwardDfsPushEx =
        [&doPush, onBackwardDfsPush](SubnetBuilder &builder,
            const bool isIn, const bool isOut, const EntryID entryID) -> bool {
          doPush = doPush && onBackwardDfsPush(builder, isIn, isOut, entryID);
          return true /* traverse all entries */;
        };
  }

  auto it = SubnetViewIterator(&view, SubnetViewIterator::BEGIN, arityProvider,
      linkProvider, onBackwardDfsPopEx ? onBackwardDfsPopEx : onBackwardDfsPop,
      onBackwardDfsPushEx ? onBackwardDfsPushEx : onBackwardDfsPush);

  while (it != view.end() && !it.isAborted) {
    ++it;
  }

  return !it.isAborted;
}

bool SubnetViewWalker::runBackward(const Visitor visitor,
                                   const bool saveEntries) {
  auto &builder = const_cast<SubnetBuilder&>(view.getParent().builder());

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

//===----------------------------------------------------------------------===//
// Subnet View Iterator
//===----------------------------------------------------------------------===//

SubnetViewIterator::SubnetViewIterator(
    const SubnetView *view,
    const Position position,
    const Visitor onBackwardDfsPop,
    const Visitor onBackwardDfsPush):
    SubnetViewIterator(view,
                       position,
                       defaultArityProvider,
                       defaultLinkProvider,
                       onBackwardDfsPop,
                       onBackwardDfsPush) {}

SubnetViewIterator::SubnetViewIterator(
    const SubnetView *view,
    const Position position,
    const ArityProvider arityProvider,
    const LinkProvider linkProvider,
    const Visitor onBackwardDfsPop,
    const Visitor onBackwardDfsPush):
    view(view),
    arityProvider(arityProvider),
    linkProvider(linkProvider),
    onBackwardDfsPop(onBackwardDfsPop),
    onBackwardDfsPush(onBackwardDfsPush) {

  isAborted = false;
  switch (position) {
  case BEGIN:
    prepareIteration();
    if (nInLeft) {
      nextPI();
    } else {
      operator++();
    }
    break;

  case END:
    entryLink = {SubnetBuilder::upperBoundID, 0};
    break;

  default:
    assert(false && "Unknown iterator type");
  }
}

SubnetViewIterator::reference SubnetViewIterator::operator*() const {
  return entryLink.first;
}

SubnetViewIterator::pointer SubnetViewIterator::operator->() const {
  return &operator*();
}

SubnetViewIterator &SubnetViewIterator::operator++() {
  auto &builder = const_cast<SubnetBuilder &>(view->getParent().builder());

  if (nInLeft) {
    nextPI();
    return *this;
  }

  if (stack.empty()) {
    assert(entryLink.first != SubnetBuilder::upperBoundID);
    entryLink = {SubnetBuilder::upperBoundID, 0};
    return *this;
  }

  bool isFinished = false;
  EntryID savedID = SubnetBuilder::invalidID;

  while (!isFinished) {
    auto &[i, j] = stack.top();
    savedID = i;
    const auto arity = arityProvider(builder, i);

    isFinished = true;
    for (; j < arity; ++j) {
      const auto link = linkProvider(builder, i, j);
      if (marked.find(link.idx) == marked.end()) {
        isFinished = false;
        UTOPIA_ON_BACKWARD_DFS_PUSH(builder, false, false, link.idx);
        stack.push({link.idx, 0});
        break;
      }
    } // for link
  }

  const auto isOut = (stack.size() <= nOutLeft);
  UTOPIA_ON_BACKWARD_DFS_POP(builder, false, isOut, savedID);

  marked.insert(savedID);
  entryLink = {savedID, 0};
  stack.pop();

  nOutLeft -= (isOut ? 1 : 0);
  return *this;
}

SubnetViewIterator SubnetViewIterator::next() const {
  return ++SubnetViewIterator(*this);
}

void SubnetViewIterator::prepareIteration() {
  const InOutMapping &iomapping = view->getInOutMapping();
  assert(iomapping.getOutNum());

  auto &builder = const_cast<SubnetBuilder &>(view->getParent().builder());

  for (const auto inputID : iomapping.inputs) {
    marked.insert(inputID.idx);
  } // for input

  for (const auto outputID : iomapping.outputs) {
    if (marked.find(outputID.idx) == marked.end()) {
      UTOPIA_ON_BACKWARD_DFS_PUSH(builder, false, true, outputID.idx);
      stack.push({outputID.idx, 0});
    } else {
      inout.insert(outputID.idx);
    }
  } // for output

  nOutLeft = stack.size();
  nInLeft = iomapping.getInNum();
}

void SubnetViewIterator::nextPI() {
  auto &builder = const_cast<SubnetBuilder &>(view->getParent().builder());

  const InOutMapping &iomapping = view->getInOutMapping();
  const SubnetSz currentIn = iomapping.getInNum() - nInLeft;
  const auto &inEntry = iomapping.getIn(currentIn);

  const auto isOut = (inout.find(inEntry.idx) != inout.end());
  UTOPIA_ON_BACKWARD_DFS_POP(builder, true, isOut, inEntry.idx);

  entryLink = {inEntry.idx, 0};
  nInLeft--;
}

} // namespace eda::gate::model
