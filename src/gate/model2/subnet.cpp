//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"
#include "gate/model2/printer/printer.h"

#include <queue>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet
//===----------------------------------------------------------------------===//

const Subnet::Link &Subnet::getLink(size_t i, size_t j) const {
  const auto &entries = getEntries();
  const auto &cell = entries[i].cell;

  if (j < Cell::InPlaceLinks) {
    return cell.link[j];
  }

  const auto k = getLinkIndices(i, j);
  return entries[k.first].link[k.second];
}

Subnet::LinkList Subnet::getLinks(size_t i) const {
  const auto &entries = getEntries();
  const auto &cell = entries[i].cell;

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

std::pair<uint32_t, uint32_t> Subnet::getPathLength() const {
  uint32_t minLength = nEntry, maxLength = 0;
  std::vector<uint32_t> min(nEntry), max(nEntry);

  const auto &entries = getEntries();
  for (size_t i = 0; i < nEntry; ++i) {
    const auto &cell = entries[i].cell;

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

EntryIterator &EntryIterator::operator--() {
  entry = builder->getPrev(entry);
  return *this;
}

EntryIterator EntryIterator::operator--(int) {
  EntryIterator copyIter(*this);
  --(*this);
  return copyIter;
}

const std::pair<size_t, size_t> SubnetBuilder::getLinkIndices(
    size_t i, size_t j) const {
  if (j < Cell::InPlaceLinks) {
    return {i, j};
  }

  auto k = j - Cell::InPlaceLinks;
  const auto n = Cell::InEntryLinks;
  size_t linkEntry = getNext(i);
  while(k > n) {
    k -= n;
    linkEntry = getNext(linkEntry);
  }
  return {linkEntry, k};
}

const Subnet::Link &SubnetBuilder::getLink(size_t i, size_t j) const {
  const auto &cell = getCell(i);
  if (j < Cell::InPlaceLinks) {
    return cell.link[j];
  }

  const auto k = getLinkIndices(i, j);
  return entries[k.first].link[k.second];
}

Subnet::LinkList SubnetBuilder::getLinks(size_t i) const {
  const auto &cell = getCell(i);
  LinkList links(cell.arity);

  size_t j = 0;
  for (; j < cell.arity && j < Subnet::Cell::InPlaceLinks; ++j) {
    links[j] = cell.link[j];
  }

  for (; j < cell.arity; ++j) {
    const auto k = getLinkIndices(i, j);
    links[j] = entries[k.first].link[k.second];
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
  const uint16_t maxCellArity = Subnet::Cell::MaxArity;
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
    SubnetID subnetID, const LinkList &links) {
  const auto &subnet = Subnet::get(subnetID);
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
      addCell(cell.getTypeID(), newLinks);
    }
  }

  return outs;
}

Subnet::Link SubnetBuilder::addSingleOutputSubnet(
    SubnetID subnetID, const LinkList &links) {
  const auto &subnet = Subnet::get(subnetID);
  assert(subnet.getOutNum() == 1);

  return addSubnet(subnetID, links).front();
}

void SubnetBuilder::replace(
    const SubnetID rhsID,
    std::unordered_map<size_t, size_t> &rhsToLhs,
    const std::function<void(const size_t)> *onNewCell) {

  const Subnet &rhs = Subnet::get(rhsID);
  assert(rhs.getOutNum() == 1);
  const auto &rhsEntries = rhs.getEntries();
  const auto lhsRootEntryID = rhsToLhs[rhsEntries.size() - 1];
  const auto oldLhsRootDepth = desc[lhsRootEntryID].depth;

  // Deleting root entry from strash
  if (rhsToLhs[0] != lhsRootEntryID) {
    destrashEntry(lhsRootEntryID);
  }

  for (size_t i = 0; i < rhsEntries.size() - 1; ++i) {
    const Subnet::Cell &rhsCell = rhsEntries[i].cell;
    assert(rhsCell.arity <= Subnet::Cell::InPlaceLinks);
    if (rhsCell.isIn()) {
      continue;
    }
    LinkList curCellLinks;
    for (const auto &link : rhs.getLinks(i)) {
      curCellLinks.push_back(Link(rhsToLhs[link.idx], link.out, link.inv));
    }

    size_t prevEntriesN = entries.size();
    size_t prevEmptyEntriesN = emptyEntryIDs.size();

    size_t newEntryID;
    bool isNewElem = false;
    if (rhs.getOut(0).idx == i && !rhs.getOut(0).inv) {
      newEntryID = replaceCell(lhsRootEntryID, rhsCell.getTypeID(),
                               curCellLinks).idx;
      isNewElem = true;
    } else {
      newEntryID = addCell(rhsCell.getTypeID(), curCellLinks).idx;
      if (prevEntriesN + 1 == entries.size() ||
          prevEmptyEntriesN == emptyEntryIDs.size() + 1) {
        isNewElem = true;
      }
    }
    rhsToLhs[i] = newEntryID;

    if (onNewCell && (isNewElem || desc[newEntryID].depth >= oldLhsRootDepth)) {
      (*onNewCell)(newEntryID);
    }
  }
  // Add extra BUF
  if ((rhs.getOut(0).idx == 0 && rhsToLhs[0] != lhsRootEntryID) ||
      rhs.getOut(0).inv) {

    LinkList bufLinks;
    CellTypeID bufTID = CELL_TYPE_ID_BUF;
    if (rhs.getOut(0).idx == 0) {
      bufLinks = {Link(rhsToLhs[0], rhs.getOut(0).out, rhs.getOut(0).inv)};
    } else {
      bufLinks = {Link(rhsToLhs[rhs.getOut(0).idx], rhs.getOut(0).out,
                  rhs.getOut(0).inv)};
    }
    replaceCell(lhsRootEntryID, bufTID, bufLinks);

    if (onNewCell) {
      (*onNewCell)(lhsRootEntryID);
    }
  }
}

std::pair<int, int> SubnetBuilder::evaluateReplace(
    const SubnetID rhsID,
    std::unordered_map<size_t, size_t> &rhsToLhs) const {

  const Subnet &rhs = Subnet::get(rhsID);
  assert(rhs.getOutNum() == 1);
  const auto &rhsEntries = rhs.getEntries();
  std::unordered_set<size_t> reusedEntries;
  auto newEntriesMetric = newEntriesEval(rhsID, rhsToLhs, reusedEntries);
  int deletedEntriesN = deletedEntriesEval(rhsToLhs[rhsEntries.size() - 1],
                                           reusedEntries);
  const int oldRootDepth = (int)desc[rhsToLhs[rhsEntries.size() - 1]].depth;
  const int newRootDepth = (int)newEntriesMetric.second;
  const size_t addedEntriesN = newEntriesMetric.first;
  return {deletedEntriesN - addedEntriesN, oldRootDepth - newRootDepth};
}

void SubnetBuilder::replaceWithZero(const EntrySet &entryIDs) {
  const size_t zeroID = addCell(ZERO).idx;
  // FIXME: Place just after inputs.
  mergeCells(MergeMap{{zeroID, entryIDs}});
}

void SubnetBuilder::replaceWithOne(const EntrySet &entryIDs) {
  const size_t oneID = addCell(ONE).idx;
  // FIXME: Place just after inputs.
  mergeCells(MergeMap{{oneID, entryIDs}});
}

std::pair<size_t, size_t> SubnetBuilder::newEntriesEval(
    const SubnetID rhsID,
    const std::unordered_map<size_t, size_t> rhsToLhs,
    std::unordered_set<size_t> &reusedEntries) const {

  int addedEntriesN = 0;
  const auto &rhs = Subnet::get(rhsID);
  const auto &rhsEntries = rhs.getEntries();
  std::vector<int> virtualDepth(rhsEntries.size(), -1);
  for (size_t rhsEntryID = 0; rhsEntryID < rhsEntries.size() - 1; ++rhsEntryID) {
    const Subnet::Cell &rhsCell = rhsEntries[rhsEntryID].cell;

    if (rhsCell.isIn()) {
      const size_t lhsEntryID = rhsToLhs.find(rhsEntryID)->second;
      reusedEntries.insert(lhsEntryID);
      virtualDepth[rhsEntryID] = (int)desc[lhsEntryID].depth;
      continue;
    }

    bool isNewElem = false;
    LinkList newRhsLinks;
    for (const auto &rhsLink : rhs.getLinks(rhsEntryID)) {
      const auto rhsLinkIdx = rhsLink.idx;
      const int rhsLinkDepth = virtualDepth[rhsLinkIdx];
      virtualDepth[rhsEntryID] = std::max(virtualDepth[rhsEntryID],
                                          rhsLinkDepth + 1);
      if (reusedEntries.find(rhsLinkIdx) == reusedEntries.end()) {
        isNewElem = true;
      } else {
        newRhsLinks.push_back(Link(*reusedEntries.find(rhsLinkIdx),
                                   rhsLink.out, rhsLink.inv));
      }
    }
    if (isNewElem) {
      ++addedEntriesN;
      continue;
    }
    StrashKey key(rhsCell.getTypeID(), newRhsLinks);
    const auto it = strash.find(key);
    if (it != strash.end()) {
      reusedEntries.insert(it->second);
    } else {
      ++addedEntriesN;
    }
  }
  return {addedEntriesN, virtualDepth[rhsEntries.size() - 2]};
}

int SubnetBuilder::deletedEntriesEval(
    const size_t rootEntryID,
    const std::unordered_set<size_t> &reusedEntries) const {
  if (reusedEntries.find(rootEntryID) != reusedEntries.end()) {
    return 0;
  }

  int deletedEntriesN = 0;
  deletedEntriesN++;
  std::unordered_map<size_t, size_t> entryNewRefcount;
  std::queue<size_t> entryIDQueue;
  size_t entryID = rootEntryID;
  entryIDQueue.push(entryID);
  while (!entryIDQueue.empty()) {
    entryID = entryIDQueue.front();
    entryIDQueue.pop();
    const auto &cell = entries[entryID].cell;
    for (size_t i = 0; i < cell.arity; ++i) {
      const auto linkIdx = cell.link[i].idx;
      if (reusedEntries.find(linkIdx) != reusedEntries.end()) {
        continue;
      }
      if (entryNewRefcount.find(linkIdx) == entryNewRefcount.end()) {
        entryNewRefcount[linkIdx] = entries[linkIdx].cell.refcount;
      }
      --entryNewRefcount[linkIdx];
      if (!entryNewRefcount[linkIdx]) {
        ++deletedEntriesN;
        entryIDQueue.push(linkIdx);
      }
    }
  }
  return deletedEntriesN;
}

Subnet::Link &SubnetBuilder::getLinkRef(size_t entryID, size_t j) {
  return const_cast<Subnet::Link&>(getLink(entryID, j));
}

void SubnetBuilder::deleteDepthBounds(size_t entryID) {
  const size_t entryDepth = desc[entryID].depth;
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
  const size_t curDepth = desc[entryID].depth;
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
    placeAfter(entryID, depthBounds[curDepth].second);
    depthBounds[curDepth].second = entryID;
  }
}

size_t SubnetBuilder::allocEntry() {
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

  size_t idx = (status.first != invalidID) ? status.first : allocEntry();

  desc[idx].depth = 0;

  for (const auto link : links) {
    desc[idx].depth = std::max(desc[idx].depth, desc[link.idx].depth + 1);
    auto &cell = getCell(link.idx);
    assert(!cell.isOut());
    cell.incRefCount();
  }
  entries[idx] = Subnet::Entry(typeID, links);

  addDepthBounds(idx);
  const size_t curDepth = desc[idx].depth;

  constexpr auto InPlaceLinks = Subnet::Cell::InPlaceLinks;
  constexpr auto InEntryLinks = Subnet::Cell::InEntryLinks;
  if (InPlaceLinks >= links.size()) {
    return idx;
  }
  const size_t saveNextEntryID = getNext(idx);
  size_t prevEntryID = idx;

  for (size_t i = InPlaceLinks; i < links.size(); i += InEntryLinks) {
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
  // Updating depth bounds
  deleteDepthBounds(entryID);
  desc[entryID].depth = invalidID;
  emptyEntryIDs.push_back(entryID);
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
        source.decRefCount();
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
    desc[secondID].prev = firstID;
  }
  if (firstID != lowerBoundID && getNext(firstID) != secondID) {
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
    const size_t curDepth = desc[curEntryID].depth;
    for (const auto &link : getLinks(curEntryID)) {
      if (toRecompute.find(link.idx) != toRecompute.end()) {
        toRecomputeN--;
      }
      newDepth = std::max(newDepth, desc[link.idx].depth + 1);
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
    link = newLinks[j];
  }
}

void SubnetBuilder::deleteCell(size_t entryID) {
  std::queue<size_t> queue;
  queue.push(entryID);

  do {
    const size_t currentID = queue.front();
    queue.pop();

    auto &cell = getCell(currentID);
    assert(cell.arity <= Subnet::Cell::InPlaceLinks);
    deallocEntry(currentID);

    for (size_t j = 0; j < cell.arity; ++j) {
      const size_t inputID = cell.link[j].idx;

      auto &inputCell = getCell(inputID);
      inputCell.decRefCount();

      if (!inputCell.refcount && !inputCell.isIn() /* leave inputs */) {
        queue.push(inputID);
      }
    }
  } while (!queue.empty());
}

Subnet::Link SubnetBuilder::replaceCell(
    size_t entryID, CellTypeID typeID, const LinkList &links) {
  assert(StrashKey::isEnabled(typeID, links));

  destrashEntry(entryID);

  const auto oldRootNext = getNext(entryID);
  const auto oldRefcount = getCell(entryID).refcount;
  const auto oldLinks = getLinks(entryID);
  const auto oldDepth = desc[entryID].depth;
  size_t newDepth = 0;

  for (const auto &link : links) {
    getCell(link.idx).incRefCount();
    newDepth = std::max(newDepth, desc[link.idx].depth + 1);
  }
  for (const auto &link : oldLinks) {
    const size_t inputEntryID = link.idx;

    auto &inputCell = getCell(inputEntryID);
    inputCell.decRefCount();
    if (!inputCell.refcount && !inputCell.isIn()) {
      deleteCell(inputEntryID);
    }
  }

  entries[entryID] = Subnet::Entry(typeID, links);
  auto &cell = getCell(entryID);
  cell.refcount = oldRefcount;
  auto it = strash.find({StrashKey(typeID, links)});
  if (it == strash.end()) {
    strash.insert({StrashKey(getCell(entryID)), entryID});
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

void SubnetBuilder::rearrangeEntries(std::vector<size_t> &newToOldEntries) {
  std::vector<Subnet::Entry> newEntries;
  std::vector<size_t> saveMapping;
  std::vector<size_t> saveDepth(entries.size(), invalidID);
  newEntries.reserve(entries.size());
  if (!newToOldEntries.empty()) {
    saveMapping.resize(newToOldEntries.size(), invalidID);
  }

  std::unordered_map<size_t, size_t> relinkMapping;
  relinkMapping.reserve(entries.size());
  bool outVisited = false;
  size_t lastCellDepth = invalidID;
  size_t isLink = 0;
  for(size_t i = getSubnetBegin(); i != upperBoundID; i = getNext(i)) {
    relinkMapping[i] = newEntries.size();
    if (isLink) {
      newEntries.push_back(entries[i]);
      isLink -= std::min(Cell::InEntryLinks, isLink);
      continue;
    }

    // Update depth and depth bounds
    saveDepth[newEntries.size()] = desc[newEntries.size()].depth;
    desc[newEntries.size()].depth = saveDepth[i] == invalidID ? desc[i].depth :
                                    saveDepth[i];
    if (lastCellDepth != invalidID && !outVisited &&
        desc[newEntries.size()].depth != lastCellDepth) {
      depthBounds[lastCellDepth].second = newEntries.size() - 1;
      if (getCell(i).getTypeID() != CELL_TYPE_ID_OUT) {
        depthBounds[desc[newEntries.size()].depth].first = newEntries.size();
      } else {
        outVisited = true;
      }
    }
    lastCellDepth = desc[newEntries.size()].depth;

    if (!newToOldEntries.empty()) {
      saveMapping[relinkMapping[i]] = newToOldEntries[relinkMapping[i]];
      if (saveMapping[i] != invalidID) {
        newToOldEntries[relinkMapping[i]] = saveMapping[i];
      } else {
        newToOldEntries[relinkMapping[i]] = newToOldEntries[i];
      }
    }

    const auto &cell = getCell(i);
    isLink += std::max(cell.arity, Cell::InPlaceLinks) - Cell::InPlaceLinks;
    //assert(cell.isIn() || cell.isOut() || cell.refcount);

    LinkList links;
    for (size_t j = 0; j < cell.arity; ++j) {
      const auto &link = getLink(i, j);
      const auto it = relinkMapping.find(link.idx);
      const auto idx = (it != relinkMapping.end()) ? it->second : link.idx;

      links.emplace_back(idx, link.out, link.inv);
    }

    relinkCell(i, links);
    newEntries.push_back(entries[i]);
  }

  entries = std::move(newEntries);
  desc.resize(entries.size());
  clearContext();
}

void SubnetBuilder::clearContext() {
  emptyEntryIDs.clear();
  subnetBegin = normalOrderID;
  subnetEnd = normalOrderID;
  for (size_t i = getSubnetBegin(); i != upperBoundID; i = getNext(i)) {
    desc[i].prev = desc[i].next = normalOrderID;
  }
  strash.clear();
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
