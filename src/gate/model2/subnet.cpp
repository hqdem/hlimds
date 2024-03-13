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

const Subnet::Link &SubnetBuilder::getLink(size_t i, size_t j) const {
  const auto &cell = getCell(i);
  if (j < Cell::InPlaceLinks) {
    return cell.link[j];
  }

  const auto k = Subnet::getLinkIndices(i, j);
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
    const auto k = Subnet::getLinkIndices(i, j);
    links[j] = entries[k.first].link[k.second];
  }

  return links;
}

Subnet::Link SubnetBuilder::addCell(CellTypeID typeID, const LinkList &links) {
  const bool isPositive = !CellType::get(typeID).isNegative();
  assert(isPositive && "Negative cells are not allowed");

  const bool in  = (typeID == CELL_TYPE_ID_IN);
  const bool out = (typeID == CELL_TYPE_ID_OUT);

  assert((!in || entries.size() == nIn)
      && "Input cells after non-input cells are not allowed");

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

  if (subnetEnd == normalOrderID && subnetBegin == normalOrderID) {
    subnetEnd = entries.size() - 1;
    subnetBegin = 0;
  }
  destrashEntry(rhsToLhs[rhsEntries.size() - 1]);

  size_t prevNewCellID = invalidID;
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
      const size_t curSubnetEnd = subnetEnd;
      newEntryID = addCell(rhsCell.getTypeID(), curCellLinks).idx;
      subnetEnd = curSubnetEnd;
      if (prevEntriesN + 1 == entries.size() ||
          prevEmptyEntriesN == emptyEntryIDs.size() + 1) {

        isNewElem = true;
      }
    }
    rhsToLhs[i] = newEntryID;

    if (!isNewElem) {
      continue;
    }
    if (prevNewCellID == invalidID) {
      setOrder(getPrev(lhsRootEntryID), newEntryID);
    } else {
      setOrder(prevNewCellID, newEntryID);
    }
    prevNewCellID = newEntryID;
    if (onNewCell) {
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
    if (prevNewCellID == invalidID) {
      setOrder(getPrev(lhsRootEntryID), lhsRootEntryID);
    } else {
      setOrder(prevNewCellID, lhsRootEntryID);
    }
    if (onNewCell) {
      (*onNewCell)(lhsRootEntryID);
    }
  }
}

int SubnetBuilder::evaluateReplace(
    const SubnetID rhsID,
    std::unordered_map<size_t, size_t> &rhsToLhs) const {

  const Subnet &rhs = Subnet::get(rhsID);
  assert(rhs.getOutNum() == 1);
  const auto &rhsEntries = rhs.getEntries();
  std::unordered_set<size_t> reusedEntries;
  int addedEntriesN = cntNewEntries(rhsID, rhsToLhs, reusedEntries);
  int deletedEntriesN = cntDeletedEntries(rhsToLhs[rhsEntries.size() - 1],
                                          reusedEntries);
  return deletedEntriesN - addedEntriesN;
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

int SubnetBuilder::cntNewEntries(
    const SubnetID rhsID,
    const std::unordered_map<size_t, size_t> rhsToLhs,
    std::unordered_set<size_t> &reusedEntries) const {

  int addedEntriesN = 0;
  const auto &rhs = Subnet::get(rhsID);
  const auto &rhsEntries = rhs.getEntries();
  for (size_t rhsEntryID = 0; rhsEntryID < rhsEntries.size() - 1; ++rhsEntryID) {
    const Subnet::Cell &rhsCell = rhsEntries[rhsEntryID].cell;

    if (rhsCell.isIn()) {
      reusedEntries.insert(rhsToLhs.find(rhsEntryID)->second);
      continue;
    }

    bool isNewElem = false;
    LinkList newRhsLinks;
    for (const auto &rhsLink : rhs.getLinks(rhsEntryID)) {
      if (rhsToLhs.find(rhsLink.idx) == rhsToLhs.end()) {
        isNewElem = true;
        break;
      }
      newRhsLinks.push_back(Link(rhsToLhs.find(rhsLink.idx)->second,
                                  rhsLink.out, rhsLink.inv));
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
  return addedEntriesN;
}

int SubnetBuilder::cntDeletedEntries(
    const size_t rootEntryID,
    const std::unordered_set<size_t> &reusedEntries) const {

  int deletedEntriesN = 0;
  if (reusedEntries.find(rootEntryID) != reusedEntries.end()) {
    return deletedEntriesN;
  }

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

size_t SubnetBuilder::allocEntry() {
  if (!emptyEntryIDs.empty()) {
    const auto allocatedID = emptyEntryIDs.back();
    emptyEntryIDs.pop_back();
    return allocatedID;
  }

  entries.resize(entries.size() + 1);
  return entries.size() - 1;
}

size_t SubnetBuilder::allocEntry(CellTypeID typeID, const LinkList &links) {
  const auto status = strashEntry(typeID, links);
  if (status.first != invalidID && !status.second) {
    return status.first;
  }

  size_t idx = (status.first != invalidID) ? status.first : allocEntry();

  for (const auto link : links) {
    auto &cell = getCell(link.idx);
    assert(!cell.isOut());
    cell.incRefCount();
  }

  entries[idx] = Subnet::Entry(typeID, links);
  if (subnetEnd != normalOrderID) {
    setOrder(subnetEnd, idx);
  }

  constexpr auto InPlaceLinks = Subnet::Cell::InPlaceLinks;
  constexpr auto InEntryLinks = Subnet::Cell::InEntryLinks;
  for (size_t i = InPlaceLinks; i < links.size(); i += InEntryLinks) {
    entries.emplace_back(links, i);
  }

  return idx;
}

void SubnetBuilder::deallocEntry(size_t entryID) {
  const auto &cell = getCell(entryID);
  assert(!cell.refcount);

  destrashEntry(entryID);
  setOrder(getPrev(entryID), getNext(entryID));
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
  if (entryID == lowerBoundID) {
    return getSubnetBegin();
  }
  assert(entryID < entries.size());
  if (entryID == getSubnetEnd()) {
    return upperBoundID;
  }
  return entryID >= next.size() || next[entryID] == normalOrderID ?
          entryID + 1 : next[entryID];
}

size_t SubnetBuilder::getPrev(const size_t entryID) const {
  if (entryID == upperBoundID) {
    return getSubnetEnd();
  }
  assert(entryID < entries.size());
  if (entryID == getSubnetBegin()) {
    return lowerBoundID;
  }
  return entryID >= prev.size() || prev[entryID] == normalOrderID ?
          entryID - 1 : prev[entryID];
}

void SubnetBuilder::setOrder(const size_t firstID, const size_t secondID) {
  assert(firstID != upperBoundID && secondID != lowerBoundID);
  assert(firstID != lowerBoundID || secondID != upperBoundID);

  if (secondID == getSubnetBegin()) {
    subnetBegin = firstID;
  } else if (firstID == lowerBoundID) {
    subnetBegin = secondID;
  }
  if (firstID == getSubnetEnd()) {
    subnetEnd = secondID;
  } else if (secondID == upperBoundID) {
    subnetEnd = firstID;
  }
  if (secondID != upperBoundID && getPrev(secondID) != firstID) {
    if (secondID >= prev.size()) {
      prev.resize(secondID + 1, normalOrderID);
    }
    prev[secondID] = firstID;
  }
  if (firstID != lowerBoundID && getNext(firstID) != secondID) {
    if (firstID >= next.size()) {
      next.resize(firstID + 1, normalOrderID);
    }
    next[firstID] = secondID;
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
  if (strash.find(StrashKey(getCell(entryID))) != strash.end() &&
      strash.find(StrashKey(getCell(entryID)))->second == entryID) {

    destrashEntry(entryID);
  }

  auto &cell = getCell(entryID);
  for (const auto &link : links) {
    getCell(link.idx).incRefCount();
  }

  Subnet::Entry newCellEntry(typeID, links);
  newCellEntry.cell.refcount = cell.refcount;
  for (size_t j = 0; j < cell.arity; ++j) {
    const size_t inputEntryID = cell.link[j].idx;

    auto &inputCell = getCell(inputEntryID);
    inputCell.decRefCount();
    if (!inputCell.refcount) {
      deleteCell(inputEntryID);
    }
  }

  newCellEntry.cell.refcount = cell.refcount;
  entries[entryID] = std::move(newCellEntry);
  auto it = strash.find({StrashKey(typeID, links)});
  if (it == strash.end()) {
    strash.insert({StrashKey(getCell(entryID)), entryID});
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

void SubnetBuilder::rearrangeEntries() {
  std::vector<Subnet::Entry> newEntries;
  newEntries.reserve(entries.size());

  std::unordered_map<size_t, size_t> relinkMapping;
  relinkMapping.reserve(entries.size());

  for(size_t i = getSubnetBegin(); i != upperBoundID; i = getNext(i)) {
    relinkMapping[i] = newEntries.size();

    const auto &cell = getCell(i);
    assert(cell.isIn() || cell.isOut() || cell.refcount);

    LinkList links;
    for (size_t j = 0; j < cell.arity; ++j) {
      const auto &link = cell.link[j];
      const auto it = relinkMapping.find(link.idx);
      const auto idx = (it != relinkMapping.end()) ? it->second : link.idx;

      links.emplace_back(idx, link.out, link.inv);
    }

    relinkCell(i, links);
    newEntries.push_back(entries[i]);
  }

  entries = std::move(newEntries);
  clearContext();
}

void SubnetBuilder::clearContext() {
  prev.clear();
  next.clear();
  emptyEntryIDs.clear();
  subnetBegin = normalOrderID;
  subnetEnd = normalOrderID;
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
