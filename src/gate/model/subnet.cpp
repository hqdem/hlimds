//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnet.h"
#include "gate/model/printer/printer.h"

#include <cstring>
#include <iostream>
#include <queue>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet
//===----------------------------------------------------------------------===//

const Subnet::Link &Subnet::getLink(size_t i, size_t j) const {
  const auto &cell = getCell(i);

  if (j < Cell::InPlaceLinks) {
    return cell.link[j];
  }

  const auto k = getLinkIndices(i, j);
  return entries[k.first].link[k.second];
}

const Subnet::LinkList Subnet::getLinks(size_t i) const {
  const auto &cell = getCell(i);
  LinkList links(cell.arity);

  size_t j = 0;
  for (; j < cell.arity && j < Cell::InPlaceLinks; ++j) {
    links[j] = cell.link[j];
  }

  for (; j < cell.arity; ++j) {
    const auto k = getLinkIndices(i, j);
    links[j] = entries[k.first].link[k.second];
  }

  return links;
}

const Subnet::Link *Subnet::getLinks(
    size_t i, Link *links, uint16_t &nLinks) const {
  const auto &cell = getCell(i);
  nLinks = cell.arity;
  // Return the pointer to the inner array.
  return cell.link;
}

std::pair<uint32_t, uint32_t> Subnet::getPathLength() const {
  uint32_t minLength = nEntry, maxLength = 0;
  std::vector<uint32_t> min(nEntry), max(nEntry);

  for (size_t i = 0; i < nEntry; ++i) {
    const auto &cell = getCell(i);

    if (cell.isIn()) {
      min[i] = max[i] = 0;
    } else {
      min[i] = nEntry; max[i] = 0;

      for (size_t j = 0; j < cell.arity; ++j) {
        auto link = getLink(i, j);
        min[i] = std::min(min[i], min[link.idx]);
        max[i] = std::max(max[i], max[link.idx]);
      }

      if (!cell.isOut()) {
        min[i]++; max[i]++;
      }
    }

    if (cell.isOut()) {
      minLength = std::min(minLength, min[i]);
      maxLength = std::max(maxLength, max[i]);
    }

    i += cell.more;
  }

  return {minLength, maxLength};
}

std::ostream &operator <<(std::ostream &out, const Subnet &subnet) {
  ModelPrinter::getDefaultPrinter().print(out, subnet);
  return out;
}

//===----------------------------------------------------------------------===//
// Subnet Builder
//===----------------------------------------------------------------------===//

inline float weight(const size_t entryID,
                    const SubnetBuilder::CellWeightProvider *provider) {
  return provider != nullptr ? (*provider)(entryID) : 0.0;
}

inline float weight(const float providedWeight,
                    const SubnetBuilder::CellWeightModifier *modifier) {
  return modifier != nullptr ? (*modifier)(providedWeight) : providedWeight;
}

inline float weight(const size_t entryID,
                    const SubnetBuilder::CellWeightProvider *provider,
                    const SubnetBuilder::CellWeightModifier *modifier) {
  return provider != nullptr ? weight((*provider)(entryID), modifier) : 0.0;
}

EntryIterator::reference EntryIterator::operator*() const {
  return entry;
}

EntryIterator::pointer EntryIterator::operator->() const {
  return &operator*();
}

EntryIterator &EntryIterator::operator++() {
  entry = builder->getNext(entry);
  return *this;
}

EntryIterator EntryIterator::operator++(int) {
  EntryIterator copyIter(*this);
  ++(*this);
  return copyIter;
}

EntryIterator EntryIterator::next() const {
  return ++EntryIterator(*this);
}

EntryIterator &EntryIterator::operator--() {
  entry = builder->getPrev(entry);
  return *this;
}

EntryIterator EntryIterator::operator--(int) {
  EntryIterator copyIter(*this);
  --(*this);
  return copyIter;
}

EntryIterator EntryIterator::prev() const {
  return --EntryIterator(*this);
}

void EntryIterator::nextCell() {
  const auto &cell = builder->getCell(entry);
  for (size_t i = 0; i <= cell.more; ++i) {
    entry = builder->getNext(entry);
  }
}

template <CellSymbol symbol>
static SubnetID makeConstSubnet(const size_t nIn) {
  constexpr size_t size{8};
  static std::vector<SubnetID> cache(size + 1, OBJ_NULL_ID);

  auto &subnetID = nIn < size ? cache[nIn] : cache[size];

  if (nIn < size && subnetID != OBJ_NULL_ID) {
    return subnetID;
  }

  SubnetBuilder builder;
  builder.addInputs(nIn);
  builder.addOutput(builder.addCell(symbol));
  return (subnetID = builder.make());
}

SubnetID SubnetBuilder::makeZero(const size_t nIn) {
  return makeConstSubnet<ZERO>(nIn);
}

SubnetID SubnetBuilder::makeOne(const size_t nIn) {
  return makeConstSubnet<ONE>(nIn);
}

SubnetID SubnetBuilder::makeConst(const size_t nIn, const bool value) {
  return value ? makeOne(nIn) : makeZero(nIn);
}

const Subnet::Link &SubnetBuilder::getLink(size_t i, size_t j) const {
  const auto &cell = getCell(i);
  if (j < Cell::InPlaceLinks) {
    return cell.link[j];
  }
  const auto k = getLinkIndices(i, j);
  return entries[k.first].link[k.second];
}

const Subnet::LinkList SubnetBuilder::getLinks(size_t i) const {
  const auto &cell = getCell(i);
  LinkList links(cell.arity);

  size_t j = 0;
  for (; j < cell.arity && j < Cell::InPlaceLinks; ++j) {
    links[j] = cell.link[j];
  }

  size_t k = 0;
  size_t n = getNext(i);

  for (; j < cell.arity; ++j) {
    links[j] = entries[n].link[k++]; 

    if (k == Cell::InEntryLinks) {
      k = 0;
      n = getNext(n);
    }
  }

  return links;
}

const Subnet::Link *SubnetBuilder::getLinks(
    size_t i, Link *links, uint16_t &nLinks) const {
  const auto &cell = getCell(i);
  nLinks = cell.arity;

  if (cell.arity <= Cell::InPlaceLinks) {
    // Return the pointer to the inner array.
    return cell.link;
  }

  // Fill the links buffer provided by a user.
  auto *buffer = links;
  auto nItems = cell.arity;

  std::memcpy(buffer, cell.link, Cell::InPlaceLinks * sizeof(Link));
  buffer += Cell::InPlaceLinks;
  nItems -= Cell::InPlaceLinks;

  size_t n = getNext(i);
  for(; nItems >= Cell::InEntryLinks; n = getNext(n)) {
    std::memcpy(buffer, entries[n].link, Cell::InEntryLinks * sizeof(Link));
    buffer += Cell::InEntryLinks;
    nItems -= Cell::InEntryLinks;
  }

  if (nItems) {
    std::memcpy(buffer, entries[n].link, nItems * sizeof(Link));
  }

  return links;
}

Subnet::Link SubnetBuilder::addCell(CellTypeID typeID, const LinkList &links) {
  const bool isPositive = !CellType::get(typeID).isNegative();
  assert(isPositive && "Negative cells are not allowed");

  const bool in  = (typeID == CELL_TYPE_ID_IN);
  const bool out = (typeID == CELL_TYPE_ID_OUT);

  const auto idx = allocEntry(typeID, links);

  if (in)  nIn++;
  if (out) nOut++;

  return Link(idx);
}

Subnet::Link SubnetBuilder::addCellTree(
    CellSymbol symbol, const LinkList &links, uint16_t k) {
  const uint16_t maxCellArity = Cell::MaxArity;
  const uint16_t maxTreeArity = (k > maxCellArity) ? maxCellArity : k;

  if (links.size() <= maxTreeArity) {
    return addCell(symbol, links);
  }

  bool isRegroupable = CellType::get(getCellTypeID(symbol)).isRegroupable();
  assert(isRegroupable && "Only regroupable cells are allowed");

  LinkList linkList = links;
  linkList.reserve(2 * links.size() - 1);

  for (size_t i = 0; i < linkList.size() - 1;) {
    const size_t nRest = linkList.size() - i;
    const size_t nArgs = (nRest > maxTreeArity) ? maxTreeArity : nRest;

    LinkList args(nArgs);
    for (size_t j = 0; j < nArgs; ++j) {
      args[j] = linkList[i++];
    }

    linkList.emplace_back(addCell(symbol, args));
  }

  return linkList.back();
}

Subnet::LinkList SubnetBuilder::addSubnet(
    const Subnet &subnet,
    const LinkList &links,
    const CellWeightProvider *weightProvider) {
  const auto &subnetEntries = subnet.getEntries();

  const auto offset = (entries.size() - subnet.getInNum());

  LinkList outs;
  outs.reserve(subnet.getOutNum());

  for (size_t i = subnet.getInNum(); i < subnetEntries.size(); ++i) {
    auto newLinks = subnet.getLinks(i);

    for (size_t j = 0; j < newLinks.size(); ++j) {
      auto &newLink = newLinks[j];
      if (newLink.idx < subnet.getInNum()) {
        const bool inv = newLink.inv;
        newLink = links[newLink.idx];
        newLink.inv ^= inv;
      } else {
        newLink.idx += offset;
      }
    }

    const auto &cell = subnetEntries[i].cell;
    i += cell.more;

    if (cell.isOut()) {
      outs.push_back(newLinks[0]);
    } else {
      // Destrash the entry if required.
      if (StrashKey::isEnabled(cell.getTypeID(), newLinks)) {
        const auto newEntryStrKey = StrashKey{cell.getTypeID(), newLinks};
        const auto existingEntryIt = strash.find(newEntryStrKey);
        if (existingEntryIt != strash.end()) {
          destrashEntry(existingEntryIt->second);
        }
      }
      const auto link = addCell(cell.getTypeID(), newLinks);
      setWeight(link.idx, weight(i, weightProvider));
    }
  }

  return outs;
}

Subnet::Link SubnetBuilder::addSingleOutputSubnet(
    const Subnet &subnet, const LinkList &links) {
  assert(subnet.getOutNum() == 1);
  return addSubnet(subnet, links).front();
}

void SubnetBuilder::replace(
    const SubnetObject &rhs,
    const InOutMapping &iomapping,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onEqualDepth,
    const CellActionCallback *onGreaterDepth) {
  // Builder is of higher priority (it contains the cell weights).
  if (rhs.hasBuilder()) {
    replace(rhs.builder(), iomapping,
        onNewCell, onEqualDepth, onGreaterDepth);
  } else {
    replace(rhs.id(), iomapping, nullptr /* weight provider */,
        onNewCell, onEqualDepth, onGreaterDepth);
  }
}

void SubnetBuilder::replace(
    const SubnetID rhsID,
    const InOutMapping &iomapping,
    const CellWeightProvider *weightProvider,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onEqualDepth,
    const CellActionCallback *onGreaterDepth) {

  const auto &rhs = Subnet::get(rhsID);
  const auto &rhsEntries = rhs.getEntries();
  replace<Subnet, Array<Entry>, ArrayIterator<Entry>>(
    rhs, rhsEntries, rhsEntries.size() - 1, iomapping,
    [&](ArrayIterator<Entry> iter, size_t i) {return i;},
    weightProvider, onNewCell, onEqualDepth, onGreaterDepth
  );
}

void SubnetBuilder::replace(
    const SubnetBuilder &rhsBuilder,
    const InOutMapping &iomapping,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onEqualDepth,
    const CellActionCallback *onGreaterDepth) {

  const CellWeightProvider weightProvider = [&](size_t i) -> float {
    return rhsBuilder.getWeight(i);
  };
  replace<SubnetBuilder, SubnetBuilder, EntryIterator>(
    rhsBuilder, rhsBuilder, *(--rhsBuilder.end()), iomapping,
    [&](EntryIterator iter, size_t i) {return *iter;},
    &weightProvider, onNewCell, onEqualDepth, onGreaterDepth
  );
}

//-- FIXME: Implement entryID iterators for Subnet.
template <typename RhsContainer>
void fillMapping(const RhsContainer &rhs,
                 const InOutMapping &iomapping,
                 std::vector<size_t> &rhsToLhs) {
  assert(false && "Specialization is required");
}

template <>
void fillMapping<Subnet>(const Subnet &rhs,
                         const InOutMapping &iomapping,
                         std::vector<size_t> &rhsToLhs) {
  assert(rhs.getInNum() == iomapping.getInNum());
  assert(rhs.getOutNum() == iomapping.getOutNum());

  for (size_t i = 0; i < iomapping.getInNum(); ++i) {
    rhsToLhs[rhs.getInIdx(i)] = iomapping.getIn(i);
  }
  for (size_t i = 0; i < iomapping.getOutNum(); ++i) {
    rhsToLhs[rhs.getOutIdx(i)] = iomapping.getOut(i);
  }
}

template <>
void fillMapping<SubnetBuilder>(const SubnetBuilder &rhs,
                                const InOutMapping &iomapping,
                                std::vector<size_t> &rhsToLhs) {
  assert(rhs.getInNum() == iomapping.getInNum());
  assert(rhs.getOutNum() == iomapping.getOutNum());

  size_t i = 0;
  for (auto it = rhs.begin(); it != rhs.end(); ++it, ++i) {
    if (!rhs.getCell(*it).isIn()) break;
    rhsToLhs[*it] = iomapping.getIn(i);
  }

  size_t j = 0;
  for (auto it = --rhs.end(); it != rhs.begin(); --it, ++j) {
    if (!rhs.getCell(*it).isOut()) break;
    rhsToLhs[*it] = iomapping.getOut(rhs.getOutNum() - 1 - j);
  }
}
//-- FIXME:

template <typename RhsContainer, typename RhsIterable, typename RhsIt>
void SubnetBuilder::replace(
    const RhsContainer &rhsContainer,
    const RhsIterable &rhsIterable,
    const size_t rhsOutEntryID,
    const InOutMapping &iomapping,
    const std::function<size_t(RhsIt iter, size_t i)> &getEntryID,
    const CellWeightProvider *weightProvider,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onEqualDepth,
    const CellActionCallback *onGreaterDepth) {

  assert(rhsContainer.getOutNum() == 1);

  std::vector<size_t> rhsToLhs(rhsContainer.getMaxIdx() + 1);
  fillMapping(rhsContainer, iomapping, rhsToLhs);

  auto rootStrashIt = strash.end();
  const auto lhsRootEntryID = rhsToLhs[rhsOutEntryID];

  const auto oldLhsRootDepth = getDepth(lhsRootEntryID);
  const auto &rhsOutLink = rhsContainer.getLink(rhsOutEntryID, 0);

  // Delete the root entry from the strash map.
  if (rhsToLhs[0] != lhsRootEntryID) {
    destrashEntry(lhsRootEntryID);
  }

  size_t i = 0;
  for (auto rhsIt = rhsIterable.begin();
       !rhsContainer.getCell(getEntryID(rhsIt, i)).isOut();
       ++rhsIt, ++i) {
    const size_t rhsEntryID = getEntryID(rhsIt, i);
    const auto &rhsCell = rhsContainer.getCell(rhsEntryID);
    assert(rhsCell.arity <= Cell::InPlaceLinks);
    if (rhsCell.isIn()) {
      continue;
    }
    LinkList curCellLinks;
    for (const auto &link : rhsContainer.getLinks(rhsEntryID)) {
      curCellLinks.push_back(Link(rhsToLhs[link.idx], link.out, link.inv));
    }

    size_t prevEntriesN = entries.size();
    size_t prevEmptyEntriesN = emptyEntryIDs.size();

    size_t newEntryID;
    bool isNewEntry = false;
    const auto curEntryStrKey = StrashKey{rhsCell.getTypeID(), curCellLinks};
    if (rhsOutLink.idx == rhsEntryID && !rhsOutLink.inv &&
        (rootStrashIt = strash.find(curEntryStrKey)) == strash.end()) {
      newEntryID = replaceCell(lhsRootEntryID, rhsCell.getTypeID(),
                               curCellLinks).idx;
      isNewEntry = true;
    } else {
      newEntryID = addCell(rhsCell.getTypeID(), curCellLinks).idx;
      if (prevEntriesN + 1 == entries.size() ||
          prevEmptyEntriesN == emptyEntryIDs.size() + 1) {
        isNewEntry = true;
      }
    }
    rhsToLhs[rhsEntryID] = newEntryID;

    // Set the weight of the new entry.
    if (isNewEntry && weightProvider) {
      setWeight(newEntryID, weight(rhsEntryID, weightProvider));
    }

    if (isNewEntry) {
      if (onNewCell) {
        (*onNewCell)(newEntryID);
      }
    } else if (getDepth(newEntryID) == oldLhsRootDepth) {
      if (onEqualDepth) {
        (*onEqualDepth)(newEntryID);
      }
    } else if (getDepth(newEntryID) > oldLhsRootDepth) {
      if (onGreaterDepth) {
        (*onGreaterDepth)(newEntryID);
      }
    }
  }
  // Add an extra buffer.
  if ((rhsOutLink.idx == 0 && rhsToLhs[0] != lhsRootEntryID) ||
      rhsOutLink.inv || rootStrashIt != strash.end()) {

    LinkList bufLinks;
    CellTypeID bufTID = CELL_TYPE_ID_BUF;
    if (rhsOutLink.idx == 0) {
      bufLinks = {Link(rhsToLhs[0], rhsOutLink.out, rhsOutLink.inv)};
    } else {
      bufLinks = {Link(rhsToLhs[rhsOutLink.idx], rhsOutLink.out,
                  rhsOutLink.inv)};
    }
    replaceCell(lhsRootEntryID, bufTID, bufLinks);

    if (onNewCell) {
      (*onNewCell)(lhsRootEntryID);
    }
  }
}

SubnetBuilder::Effect SubnetBuilder::evaluateReplace(
    const SubnetObject &rhs,
    const InOutMapping &iomapping,
    const CellWeightModifier *weightModifier) const {
  // Builder is of higher priority (it contains the cell weights).
  if (rhs.hasBuilder()) {
    return evaluateReplace(rhs.builder(), iomapping, weightModifier);
  }
  assert(!weightModifier && "Weight modifier is used w/o weight provider");
  return evaluateReplace(rhs.id(), iomapping, nullptr, nullptr);
}

SubnetBuilder::Effect SubnetBuilder::evaluateReplace(
    const SubnetID rhsID,
    const InOutMapping &iomapping,
    const CellWeightProvider *weightProvider,
    const CellWeightModifier *weightModifier) const {

  const auto &rhs = Subnet::get(rhsID);
  const auto &rhsEntries = rhs.getEntries();
  return evaluateReplace<Subnet>(rhs, rhsEntries.size() - 1,
      iomapping, weightProvider, weightModifier);
}

SubnetBuilder::Effect SubnetBuilder::evaluateReplace(
    const SubnetBuilder &rhsBuilder,
    const InOutMapping &iomapping,
    const CellWeightModifier *weightModifier) const {

  const size_t rhsOutEntryID = *rhsBuilder.rbegin();
  const CellWeightProvider weightProvider = [&](size_t i) -> float {
    return rhsBuilder.getWeight(i);
  };
  return evaluateReplace<SubnetBuilder>(rhsBuilder, rhsOutEntryID,
      iomapping, &weightProvider, weightModifier);
}

template <typename RhsContainer>
SubnetBuilder::Effect SubnetBuilder::evaluateReplace(
    const RhsContainer &rhsContainer,
    const size_t rhsOutEntryID,
    const InOutMapping &iomapping,
    const CellWeightProvider *weightProvider,
    const CellWeightModifier *weightModifier) const {
  assert(rhsContainer.getOutNum() == 1);
  std::unordered_set<size_t> reusedLhsEntries;
  const auto addEffect = newEntriesEval(rhsContainer, iomapping,
      reusedLhsEntries, weightProvider, weightModifier);
  const auto delEffect = deletedEntriesEval(iomapping.getOut(0),
      reusedLhsEntries, weightModifier);

  return delEffect - addEffect;
}

void SubnetBuilder::replaceWithZero(const EntrySet &entryIDs) {
  const size_t zeroID = addCell(ZERO).idx;
  mergeCells(MergeMap{{zeroID, entryIDs}});
}

void SubnetBuilder::replaceWithOne(const EntrySet &entryIDs) {
  const size_t oneID = addCell(ONE).idx;
  mergeCells(MergeMap{{oneID, entryIDs}});
}

void SubnetBuilder::enableFanouts() {
  fanoutsEnabled = true;
  fanouts.reserve(entries.size());
  for (size_t i = getSubnetBegin(); i != upperBoundID && i != invalidID;
       i = getNext(i)) {
    const auto &links = getLinks(i);
    for (const auto &link : links) {
      addFanout(link.idx, i);
    }
  }
}

void SubnetBuilder::disableFanouts() {
  fanoutsEnabled = false;
  fanouts.clear();
}

SubnetBuilder::Effect SubnetBuilder::newEntriesEval(
    const Subnet &rhs,
    const InOutMapping &iomapping,
    std::unordered_set<size_t> &reusedLhsEntries,
    const CellWeightProvider *weightProvider,
    const CellWeightModifier *weightModifier) const {

  const auto &rhsEntries = rhs.getEntries();
  return newEntriesEval<Subnet, Array<Entry>, ArrayIterator<Entry>>(
    rhs, rhsEntries, iomapping,
    [&](ArrayIterator<Entry> iter, size_t i) {return i;},
    reusedLhsEntries, weightProvider, weightModifier
  );
}

SubnetBuilder::Effect SubnetBuilder::newEntriesEval(
    const SubnetBuilder &rhsBuilder,
    const InOutMapping &iomapping,
    std::unordered_set<size_t> &reusedLhsEntries,
    const CellWeightProvider *weightProvider,
    const CellWeightModifier *weightModifier) const {

  return newEntriesEval<SubnetBuilder, SubnetBuilder, EntryIterator>(
    rhsBuilder, rhsBuilder, iomapping,
    [&](EntryIterator iter, size_t i) {return *iter;},
    reusedLhsEntries, weightProvider, weightModifier
  );
}

template <typename RhsContainer, typename RhsIterable, typename RhsIt>
SubnetBuilder::Effect SubnetBuilder::newEntriesEval(
    const RhsContainer &rhsContainer,
    const RhsIterable &rhsIterable,
    const InOutMapping &iomapping,
    const std::function<size_t(RhsIt iter, size_t i)> &getEntryID, // FIXME: Add iterator to Subnet
    std::unordered_set<size_t> &reusedLhsEntries,
    const CellWeightProvider *weightProvider,
    const CellWeightModifier *weightModifier) const {

  int addedEntriesN = 0;
  float addedWeight = 0.0;

  std::vector<int> virtualDepth(rhsContainer.getMaxIdx() + 1);

  std::vector<size_t> rhsToLhs(rhsContainer.getMaxIdx() + 1, -1u);
  fillMapping(rhsContainer, iomapping, rhsToLhs);

  size_t rhsRootEntryID = invalidID;
  size_t i = 0;
  for (auto rhsIt = rhsIterable.begin();
       !rhsContainer.getCell(getEntryID(rhsIt, i)).isOut();
       ++rhsIt, ++i) {

    const size_t rhsEntryID = getEntryID(rhsIt, i);
    rhsRootEntryID = rhsEntryID;
    const auto &rhsCell = rhsContainer.getCell(rhsEntryID);
    if (rhsCell.isIn()) {
      const auto lhsEntryID = rhsToLhs[rhsEntryID];
      reusedLhsEntries.insert(lhsEntryID);
      virtualDepth[rhsEntryID] = static_cast<int>(getDepth(lhsEntryID));
      continue;
    }

    bool isNewElem = false;
    LinkList newRhsLinks;
    for (const auto &rhsLink : rhsContainer.getLinks(rhsEntryID)) {
      const auto rhsLinkIdx = rhsLink.idx;
      const int rhsLinkDepth = virtualDepth[rhsLinkIdx];
      virtualDepth[rhsEntryID] = std::max(virtualDepth[rhsEntryID],
                                          rhsLinkDepth + 1);
      if (rhsToLhs[rhsLinkIdx] == -1u) {
        isNewElem = true;
      } else {
        newRhsLinks.push_back(Link(rhsToLhs[rhsLinkIdx],
                                   rhsLink.out, rhsLink.inv));
      }
    }
    if (isNewElem) {
      ++addedEntriesN;
      addedWeight += weight(rhsEntryID, weightProvider, weightModifier);
      continue;
    }
    StrashKey key(rhsCell.getTypeID(), newRhsLinks);
    const auto it = strash.find(key);
    if (it != strash.end()) {
      rhsToLhs[rhsEntryID] = it->second;
      reusedLhsEntries.insert(it->second);
    } else {
      ++addedEntriesN;
      addedWeight += weight(rhsEntryID, weightProvider, weightModifier);
    }
  }
  return Effect{addedEntriesN, virtualDepth[rhsRootEntryID], addedWeight};
}

SubnetBuilder::Effect SubnetBuilder::deletedEntriesEval(
    const size_t lhsRootEntryID,
    std::unordered_set<size_t> &reusedLhsEntries,
    const CellWeightModifier *weightModifier) const {
  if (reusedLhsEntries.find(lhsRootEntryID) != reusedLhsEntries.end()) {
    Effect effect;
    effect.depth = static_cast<int>(getDepth(lhsRootEntryID));
    return effect;
  }

  int deletedEntriesN = 0;
  float deletedWeight = 0.0;
  deletedEntriesN++;
  deletedWeight += weight(getWeight(lhsRootEntryID), weightModifier);
  std::unordered_map<size_t, size_t> entryNewRefcount;
  std::queue<size_t> entryIDQueue;
  size_t entryID = lhsRootEntryID;
  entryIDQueue.push(entryID);
  while (!entryIDQueue.empty()) {
    entryID = entryIDQueue.front();
    entryIDQueue.pop();
    const auto &cell = entries[entryID].cell;
    for (size_t i = 0; i < cell.arity; ++i) {
      const auto linkIdx = cell.link[i].idx;
      if (reusedLhsEntries.find(linkIdx) != reusedLhsEntries.end()) {
        continue;
      }
      if (entryNewRefcount.find(linkIdx) == entryNewRefcount.end()) {
        entryNewRefcount[linkIdx] = entries[linkIdx].cell.refcount;
      }
      --entryNewRefcount[linkIdx];
      if (!entryNewRefcount[linkIdx]) {
        ++deletedEntriesN;
        deletedWeight += weight(getWeight(linkIdx), weightModifier);
        entryIDQueue.push(linkIdx);
      }
    }
  }

  const int oldRootDepth = static_cast<int>(getDepth(lhsRootEntryID));
  return Effect{deletedEntriesN, oldRootDepth, deletedWeight};
}

Subnet::Link &SubnetBuilder::getLinkRef(size_t entryID, size_t j) {
  return const_cast<Subnet::Link&>(getLink(entryID, j));
}

void SubnetBuilder::deleteDepthBounds(size_t entryID) {
  const size_t entryDepth = getDepth(entryID);
  assert(depthBounds.size() > entryDepth);
  if (depthBounds[entryDepth].first == depthBounds[entryDepth].second) {
    depthBounds[entryDepth].first = depthBounds[entryDepth].second = invalidID;
  } else if (depthBounds[entryDepth].first == entryID) {
    depthBounds[entryDepth].first = getNext(entryID);
  } else if (depthBounds[entryDepth].second == entryID) {
    depthBounds[entryDepth].second = getPrev(entryID);
  }
  setOrder(getPrev(entryID), getNext(entryID));
}

void SubnetBuilder::addDepthBounds(size_t entryID) {
  const auto &cell = getCell(entryID);
  const auto typeID = cell.getTypeID();
  const size_t curDepth = getDepth(entryID);
  if (depthBounds.size() <= curDepth) {
    depthBounds.resize(curDepth + 1, {invalidID, invalidID});
  }
  if (typeID == CELL_TYPE_ID_OUT) {
    placeAfter(entryID, getSubnetEnd());
  } else if (depthBounds[curDepth].first == invalidID) {
    depthBounds[curDepth].first = depthBounds[curDepth].second = entryID;
    if (!curDepth) {
      placeAfter(entryID, lowerBoundID);
    } else {
      placeAfter(entryID, depthBounds[curDepth - 1].second);
    }
  } else {
    size_t entryToPlaceAfter = depthBounds[curDepth].second;
    if (!curDepth && typeID == CELL_TYPE_ID_IN) {
      auto cellToPlaceAfter = getCell(entryToPlaceAfter);
      while (entryToPlaceAfter != lowerBoundID &&
             (cellToPlaceAfter.getTypeID() == CELL_TYPE_ID_ZERO ||
              cellToPlaceAfter.getTypeID() == CELL_TYPE_ID_ONE)) {

        entryToPlaceAfter = getPrev(entryToPlaceAfter);
        cellToPlaceAfter = getCell(entryToPlaceAfter);
      }
    }
    placeAfter(entryID, entryToPlaceAfter);
    if (depthBounds[curDepth].second == entryToPlaceAfter) {
      depthBounds[curDepth].second = entryID;
    }
  }
}

void SubnetBuilder::addFanout(size_t sourceID, size_t fanoutID) {
  assert(sourceID < entries.size());
  assert(fanoutID < entries.size());
  if (!fanoutsEnabled) {
    return;
  }
  if (fanouts.size() <= sourceID) {
    fanouts.resize(sourceID + 1);
  }
  fanouts[sourceID].push_back(fanoutID);
}

void SubnetBuilder::delFanout(size_t sourceID, size_t fanoutID) {
  assert(sourceID < entries.size());
  assert(fanoutID < entries.size());
  if (!fanoutsEnabled) {
    return;
  }
  auto &srcFanouts = fanouts[sourceID];
  for (auto it = srcFanouts.begin(); it != srcFanouts.end(); ++it) {
    if ((*it) == fanoutID) {
      srcFanouts.erase(it);
      break;
    }
  }
}

size_t SubnetBuilder::allocEntry() {
  nCell++;

  if (!emptyEntryIDs.empty()) {
    const auto allocatedID = emptyEntryIDs.back();
    emptyEntryIDs.pop_back();
    return allocatedID;
  }

  entries.resize(entries.size() + 1);
  desc.resize(entries.size());

  return entries.size() - 1;
}

size_t SubnetBuilder::allocEntry(CellTypeID typeID, const LinkList &links) {
  // Fixating subnet begin and subnet end
  if (subnetBegin == normalOrderID) {
    subnetBegin = getSubnetBegin();
  }
  if (subnetEnd == normalOrderID) {
    subnetEnd = getSubnetEnd();
  }
  const auto status = strashEntry(typeID, links);
  if (status.first != invalidID && !status.second) {
    return status.first;
  }

  const size_t idx = (status.first != invalidID) ? status.first : allocEntry();

  desc[idx].depth = 0;
  desc[idx].session = 0;

  for (const auto link : links) {
    desc[idx].depth = std::max(getDepth(idx), getDepth(link.idx) + 1);
    auto &cell = getCell(link.idx);
    assert(!cell.isOut());
    addFanout(link.idx, idx);
    cell.incRefCount();
  }
  entries[idx] = Entry(typeID, links);

  addDepthBounds(idx);

  const size_t curDepth = getDepth(idx);
  const size_t nLinks = links.size();

  if (Cell::InPlaceLinks >= nLinks) {
    return idx;
  }

  const size_t saveNextEntryID = getNext(idx);
  size_t prevEntryID = idx;

  for (size_t i = Cell::InPlaceLinks; i < nLinks; i += Cell::InEntryLinks) {
    if (depthBounds[curDepth].second == entries.size() - 1) {
      depthBounds[curDepth].second = entries.size();
    }
    entries.emplace_back(links, i);
    desc.resize(entries.size());
    setOrder(prevEntryID, entries.size() - 1);
    prevEntryID = entries.size() - 1;
  }
  setOrder(entries.size() - 1, saveNextEntryID);

  return idx;
}

void SubnetBuilder::deallocEntry(size_t entryID) {
  const auto &cell = getCell(entryID);
  assert(!cell.refcount);

  destrashEntry(entryID);

  // Updating depth bounds.
  deleteDepthBounds(entryID);
  desc[entryID].depth = invalidID;
  emptyEntryIDs.push_back(entryID);

  nCell--;
}

void SubnetBuilder::mergeCells(const MergeMap &entryIDs) {
  size_t refcount = 0;

  std::unordered_map<size_t, size_t> mergeTo;
  for (const auto &[entryID, otherIDs] : entryIDs) {
    assert(getCell(entryID).getOutNum() == 1);
    assert(otherIDs.find(entryID) == otherIDs.end());

    for (const auto otherID : otherIDs) {
      assert(getCell(otherID).getOutNum() == 1);
      mergeTo.insert({otherID, entryID});
      refcount += getCell(otherID).refcount;
    }
  }
  assert(refcount);

  // Skip the entries precedings the ones being removed.
  auto i = begin();
  for (; i != end() && mergeTo.find(*i) == mergeTo.end(); ++i);

  if (i != end()) ++i;
  for (; refcount && i != end(); ++i) {
    auto &target = getCell(*i);
    for (size_t j = 0; j < target.arity; ++j) {
      auto &link = getLinkRef(*i, j);

      const auto r = mergeTo.find(link.idx);
      if (r != mergeTo.end()) {
        // The remaining entry should not depend on the entry being removed.
        auto &remain = getCell(r->second);
        auto &source = getCell(link.idx);

        // Redirect the link to the remaining cell.
        link.idx = r->second;
        delFanout(link.idx, *i);
        source.decRefCount();
        addFanout(r->second, *i);
        remain.incRefCount();

        if (!--refcount) {
          break;
        }
      }
    } // for links
  } // for cells

  // Remove the given cells.
  for (const auto other : mergeTo) {
    assert(!getCell(other.first).refcount);
    deleteCell(other.first);
  }
}

size_t SubnetBuilder::getNext(const size_t entryID) const {
  assert(entryID != upperBoundID && entryID != invalidID &&
         entryID != normalOrderID);
  if (entryID == lowerBoundID) {
    return (getSubnetBegin() == invalidID) ? upperBoundID : getSubnetBegin();
  }
  assert(entryID < entries.size());
  if (entryID == getSubnetEnd()) {
    return upperBoundID;
  }
  return desc[entryID].next == normalOrderID ? entryID + 1 : desc[entryID].next;
}

size_t SubnetBuilder::getPrev(const size_t entryID) const {
  assert(entryID != lowerBoundID && entryID != invalidID &&
         entryID != normalOrderID);
  if (entryID == upperBoundID) {
    return (getSubnetBegin() == invalidID) ? lowerBoundID : getSubnetEnd();
  }
  assert(entryID < entries.size());
  if (entryID == getSubnetBegin()) {
    return lowerBoundID;
  }
  return desc[entryID].prev == normalOrderID ? entryID - 1 : desc[entryID].prev;
}

void SubnetBuilder::setOrder(const size_t firstID, const size_t secondID) {
  assert(firstID != upperBoundID && secondID != lowerBoundID);
  assert(firstID != invalidID && firstID != normalOrderID &&
         secondID != invalidID && secondID != normalOrderID);
  if (firstID == lowerBoundID && secondID == upperBoundID) {
    subnetBegin = subnetEnd = invalidID;
    return;
  }

  if (secondID == getSubnetBegin() && firstID != lowerBoundID) {
    subnetBegin = firstID;
  } else if (firstID == lowerBoundID) {
    subnetBegin = secondID;
  }
  if (firstID == getSubnetEnd() && secondID != upperBoundID) {
    subnetEnd = secondID;
  } else if (secondID == upperBoundID) {
    subnetEnd = firstID;
  }
  if (secondID != upperBoundID && getPrev(secondID) != firstID) {
    isDisassembled = true;
    desc[secondID].prev = firstID;
  }
  if (firstID != lowerBoundID && getNext(firstID) != secondID) {
    isDisassembled = true;
    desc[firstID].next = secondID;
  }
}

void SubnetBuilder::placeAfter(size_t entryID, size_t pivotEntryID) {
  assert(pivotEntryID != upperBoundID);
  setOrder(entryID, getNext(pivotEntryID));
  setOrder(pivotEntryID, entryID);
}

void SubnetBuilder::placeBefore(size_t entryID, size_t pivotEntryID) {
  assert(pivotEntryID != lowerBoundID);
  setOrder(getPrev(pivotEntryID), entryID);
  setOrder(entryID, pivotEntryID);
}

void SubnetBuilder::recomputeFanoutDepths(size_t rootEntryID,
                                          size_t oldRootNextEntryID) {
  const auto &rootCell = getCell(rootEntryID);
  if (!rootCell.refcount) {
    return;
  }

  std::unordered_set<size_t> toRecompute;
  toRecompute.insert(rootEntryID);
  size_t toRecomputeN = rootCell.refcount;
  size_t curEntryID = oldRootNextEntryID;
  while (toRecomputeN) {
    if (toRecompute.find(curEntryID) != toRecompute.end()) {
      curEntryID = getNext(curEntryID);
      continue;
    }
    const auto &curCell = getCell(curEntryID);
    size_t newDepth = 0;
    const size_t curDepth = getDepth(curEntryID);
    for (const auto &link : getLinks(curEntryID)) {
      if (toRecompute.find(link.idx) != toRecompute.end()) {
        toRecomputeN--;
      }
      newDepth = std::max(newDepth, getDepth(link.idx) + 1);
    }
    if (newDepth == curDepth || curCell.getTypeID() == CELL_TYPE_ID_OUT) {
      desc[curEntryID].depth = newDepth;
      curEntryID = getNext(curEntryID);
      continue;
    }
    toRecomputeN += curCell.refcount;
    toRecompute.insert(curEntryID);
    size_t nextEntryID = getNext(curEntryID);
    // Changing topological order
    deleteDepthBounds(curEntryID);
    desc[curEntryID].depth = newDepth;
    addDepthBounds(curEntryID);
    curEntryID = nextEntryID;
  }
}

void SubnetBuilder::relinkCell(size_t entryID, const LinkList &newLinks) {
  auto &cell = getCell(entryID);
  assert(cell.arity == newLinks.size());

  for (size_t j = 0; j < cell.arity; ++j) {
    auto &link = getLinkRef(entryID, j);
    delFanout(link.idx, entryID);
    link = newLinks[j];
    addFanout(link.idx, entryID);
  }
}

void SubnetBuilder::deleteCell(size_t entryID) {
  std::queue<size_t> queue;
  queue.push(entryID);

  do {
    const size_t currentID = queue.front();
    queue.pop();

    auto &cell = getCell(currentID);
    assert(cell.arity <= Cell::InPlaceLinks);
    deallocEntry(currentID);

    for (size_t j = 0; j < cell.arity; ++j) {
      const size_t inputID = cell.link[j].idx;

      auto &inputCell = getCell(inputID);
      delFanout(inputID, currentID);
      inputCell.decRefCount();

      if (!inputCell.refcount && !inputCell.isIn() /* leave inputs */) {
        queue.push(inputID);
      }
    }
  } while (!queue.empty());
}

Subnet::Link SubnetBuilder::replaceCell(
    size_t entryID, CellTypeID typeID, const LinkList &links,
    bool delZeroRefcount) {
  assert(StrashKey::isEnabled(typeID, links));

  destrashEntry(entryID);

  const auto oldRootStrKey = StrashKey(getCell(entryID));
  const auto oldRootNext = getNext(entryID);
  const auto oldRefcount = getCell(entryID).refcount;
  const auto oldLinks = getLinks(entryID);
  const auto oldDepth = getDepth(entryID);
  size_t newDepth = 0;

  for (const auto &link : links) {
    addFanout(link.idx, entryID);
    getCell(link.idx).incRefCount();
    newDepth = std::max(newDepth, getDepth(link.idx) + 1);
  }
  for (const auto &link : oldLinks) {
    auto &inputCell = getCell(link.idx);
    delFanout(link.idx, entryID);
    inputCell.decRefCount();
    if (!inputCell.refcount && !inputCell.isIn() && delZeroRefcount) {
      deleteCell(link.idx);
    }
  }

  entries[entryID] = Entry(typeID, links);
  auto &cell = getCell(entryID);
  cell.refcount = oldRefcount;
  const auto newRootStrKey = StrashKey(typeID, links);
  auto it = strash.find({newRootStrKey});
  if (it == strash.end()) {
    strash.insert({StrashKey(getCell(entryID)), entryID});
  }
  if (newRootStrKey != oldRootStrKey) {
    desc[entryID].session = 0;
  }
  if (oldDepth != newDepth) {
    deleteDepthBounds(entryID);
    desc[entryID].depth = newDepth;
    addDepthBounds(entryID);
    recomputeFanoutDepths(entryID, oldRootNext);
  }

  return Link(entryID);
}

bool SubnetBuilder::checkInputsOrder() const {
  for (size_t i = 0; i < nIn; ++i) {
    const auto &cell = getCell(i);

    if (!cell.isIn()) {
      return false;
    }

    i += cell.more;
  }
  return true;
}

bool SubnetBuilder::checkOutputsOrder() const {
  for (size_t i = entries.size() - nOut; i < entries.size(); ++i) {
    const auto &cell = getCell(i);

    if (!cell.isOut()) {
      return false;
    }

    i += cell.more;
  }
  return true;
}

void SubnetBuilder::rearrangeEntries(
    std::vector<size_t> &entryMapping,
    const bool deleteBufs) {
  std::vector<Entry> newEntries;
  std::vector<EntryDescriptor> newDesc;
  std::vector<size_t> saveMapping;
  newEntries.reserve(entries.size());
  newDesc.reserve(desc.size());
  if (!entryMapping.empty()) {
    saveMapping.resize(entryMapping.size(), invalidID);
  }

  std::unordered_map<size_t, std::pair<size_t, uint32_t>> relinkMapping;
  relinkMapping.reserve(entries.size());
  bool outVisited = false;
  size_t lastCellDepth = invalidID;
  size_t isLink = 0;
  for(size_t i = getSubnetBegin(); i != upperBoundID; i = getNext(i)) {
    // Delete buffers.
    if (deleteBufs && !isLink && getCell(i).isBuf()) {
      const auto bufLink = getLinks(i)[0];
      const auto bufLinkRelinkIt = relinkMapping.find(bufLink.idx);
      const auto bufLinkRelinkIdx = bufLinkRelinkIt->second.first;
      const auto bufLinkRelinkInv = bufLinkRelinkIt->second.second;
      if (bufLinkRelinkIt != relinkMapping.end()) {
        relinkMapping[i] = {bufLinkRelinkIdx, bufLinkRelinkInv ^ bufLink.inv};
      } else {
        relinkMapping[i] = {bufLink.idx, bufLink.inv};
      }
      continue;
    }
    relinkMapping[i] = {newEntries.size(), 0};

    // Add a link entry.
    if (isLink) {
      newEntries.push_back(entries[i]);
      newDesc.push_back(EntryDescriptor());
      isLink -= std::min(Cell::InEntryLinks, isLink);
      continue;
    }

    // Find new links; update the input refcounts and descriptor.
    const auto &cell = getCell(i);
    isLink += std::max(cell.arity, Cell::InPlaceLinks) - Cell::InPlaceLinks;
    EntryDescriptor newCellDesc;
    newCellDesc.weight = getWeight(i);
    newCellDesc.depth = 0;
    const auto &oldLinks = getLinks(i);
    LinkList newLinks;
    for (size_t j = 0; j < oldLinks.size(); ++j) {
      const auto &link = oldLinks[j];
      if (j < cell.arity) {
        const auto it = relinkMapping.find(link.idx);
        const auto idx = (it != relinkMapping.end()) ?
            it->second.first : link.idx;
        const auto inv = (it != relinkMapping.end()) ?
            it->second.second ^ link.inv : link.inv;
        newLinks.emplace_back(idx, link.out, inv);
      }
      const auto newLinkIdx = relinkMapping[link.idx].first;
      newCellDesc.depth = std::max(newCellDesc.depth,
                                   newDesc[newLinkIdx].depth + 1);
      newEntries[newLinkIdx].cell.refcount++;
    }

    // Update the depth and depth bounds.
    if (lastCellDepth != invalidID && !outVisited &&
        desc[newEntries.size()].depth != lastCellDepth) {
      depthBounds[lastCellDepth].second = newEntries.size() - 1;
      if (getCell(i).getTypeID() != CELL_TYPE_ID_OUT) {
        depthBounds[newCellDesc.depth].first = newEntries.size();
      } else {
        outVisited = true;
      }
    }
    lastCellDepth = newCellDesc.depth;

    // Add a new cell entry.
    relinkCell(i, newLinks);
    newEntries.push_back(entries[i]);
    newDesc.push_back(newCellDesc);
    newEntries.back().cell.refcount = 0;

    // Update the mapping keys.
    if (!entryMapping.empty()) {
      saveMapping[relinkMapping[i].first] =
          entryMapping[relinkMapping[i].first];
      if (saveMapping[i] != invalidID) {
        entryMapping[relinkMapping[i].first] = saveMapping[i];
      } else {
        entryMapping[relinkMapping[i].first] = entryMapping[i];
      }
    }
  }

  entries = std::move(newEntries);
  desc = std::move(newDesc);
  clearContext();
}

void SubnetBuilder::clearContext() {
  emptyEntryIDs.clear();
  subnetBegin = normalOrderID;
  subnetEnd = normalOrderID;
  strash.clear();
  isDisassembled = false;
  disableFanouts();
  sessionID = 0;
  isSessionStarted = false;
}

std::pair<size_t, bool> SubnetBuilder::strashEntry(
    CellTypeID typeID, const LinkList &links) {
  if (StrashKey::isEnabled(typeID, links)) {
    const StrashKey key(typeID, links);
    const auto i = strash.find(key);

    if (i != strash.end()) {
      return {i->second, false /* old */};
    }

    const auto idx = allocEntry();
    strash.insert({key, idx});

    return {idx, true /* new */};
  }

  return {invalidID, false};
}

void SubnetBuilder::destrashEntry(size_t entryID) {
  const auto &cell = getCell(entryID);

  if (StrashKey::isEnabled(cell)) {
    const StrashKey key(cell);
    const auto i = strash.find(key);

    if (i != strash.end() && i->second == entryID) {
      strash.erase(i);
    }
  }
}

} // namespace eda::gate::model
