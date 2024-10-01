//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnet.h"
#include "gate/model/printer/net_printer.h"

#include <cstring>
#include <queue>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet
//===----------------------------------------------------------------------===//

const Subnet::Link &Subnet::getLink(uint32_t i, uint16_t j) const {
  const auto &cell = getCell(i);

  if (j < Cell::InPlaceLinks) {
    return cell.link[j];
  }

  const auto k = getLinkIndices(i, j);
  return entries[k.first].link[k.second];
}

const Subnet::LinkList Subnet::getLinks(uint32_t i) const {
  const auto &cell = getCell(i);
  LinkList links(cell.arity);

  uint16_t j = 0;
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
    uint32_t i, Link *links, uint16_t &nLinks) const {
  const auto &cell = getCell(i);
  nLinks = cell.arity;
  // Return the pointer to the inner array.
  return cell.link;
}

std::pair<uint32_t, uint32_t> Subnet::getPathLength() const {
  uint32_t minLength = nEntry, maxLength = 0;
  std::vector<uint32_t> min(nEntry), max(nEntry);

  for (uint32_t i = 0; i < nEntry; ++i) {
    const auto &cell = getCell(i);

    if (cell.arity == 0) {
      min[i] = max[i] = 0;
    } else {
      min[i] = nEntry; max[i] = 0;

      for (uint16_t j = 0; j < cell.arity; ++j) {
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
  print(out, Format::DEBUG, subnet);
  return out;
}

//===----------------------------------------------------------------------===//
// Subnet Builder
//===----------------------------------------------------------------------===//

inline float weight(const EntryID entryID,
                    const SubnetBuilder::CellWeightProvider *provider) {
  return provider != nullptr ? (*provider)(entryID)
                             : 0.0;
}

inline float weight(const float providedWeight,
                    const uint16_t fanout,
                    const SubnetBuilder::CellWeightModifier *modifier) {
  return modifier != nullptr ? (*modifier)(providedWeight, fanout)
                             : providedWeight;
}

inline float weight(const EntryID entryID,
                    const uint16_t fanout,
                    const SubnetBuilder::CellWeightProvider *provider,
                    const SubnetBuilder::CellWeightModifier *modifier) {
  return provider != nullptr ? weight((*provider)(entryID), fanout, modifier)
                             : 0.0;
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
  for (uint16_t i = 0; i <= cell.more; ++i) {
    entry = builder->getNext(entry);
  }
}

template <CellSymbol symbol>
static SubnetID makeConstSubnet(const uint16_t nIn) {
  constexpr uint16_t size{8};
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

SubnetID SubnetBuilder::makeZero(const uint16_t nIn) {
  return makeConstSubnet<ZERO>(nIn);
}

SubnetID SubnetBuilder::makeOne(const uint16_t nIn) {
  return makeConstSubnet<ONE>(nIn);
}

SubnetID SubnetBuilder::makeConst(const uint16_t nIn, const bool value) {
  return value ? makeOne(nIn) : makeZero(nIn);
}

const Subnet::Link &SubnetBuilder::getLink(EntryID i, uint16_t j) const {
  const auto &cell = getCell(i);
  if (j < Cell::InPlaceLinks) {
    return cell.link[j];
  }
  const auto k = getLinkIndices(i, j);
  return entries[k.first].link[k.second];
}

const Subnet::LinkList SubnetBuilder::getLinks(EntryID i) const {
  const auto &cell = getCell(i);
  LinkList links(cell.arity);

  uint16_t j = 0;
  for (; j < cell.arity && j < Cell::InPlaceLinks; ++j) {
    links[j] = cell.link[j];
  }

  uint16_t k = 0;
  EntryID n = getNext(i);

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
    EntryID i, Link *links, uint16_t &nLinks) const {
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

  EntryID n = getNext(i);
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

Subnet::LinkList SubnetBuilder::addMultiOutputCell(
    CellTypeID typeID, const LinkList &links) {
  const auto &type = CellType::get(typeID);
  LinkList result(type.getOutNum());

  result[0] = addCell(typeID, links);
  const auto idx = result[0].idx;

  for (uint16_t i = 1; i < type.getOutNum(); ++i) {
    result[i] = Link(idx, i, false);
  }

  return result;
}

Subnet::Link SubnetBuilder::addCell(CellTypeID typeID, const LinkList &links) {
  assert(!CellType::get(typeID).isNegative()
      && "Negative cells are not allowed");

  const auto in  = (typeID == CELL_TYPE_ID_IN);
  const auto out = (typeID == CELL_TYPE_ID_OUT);

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

  for (uint32_t i = 0; i < linkList.size() - 1;) {
    const uint32_t nRest = linkList.size() - i;
    const uint16_t nArgs = (nRest > maxTreeArity) ? maxTreeArity : nRest;

    LinkList args(nArgs);
    for (uint16_t j = 0; j < nArgs; ++j) {
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

  for (uint32_t i = subnet.getInNum(); i < subnetEntries.size(); ++i) {
    auto newLinks = subnet.getLinks(i);

    for (uint16_t j = 0; j < newLinks.size(); ++j) {
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
    const CellActionCallback *onGreaterDepth,
    const CellActionCallback *onRecomputedDetph) {
  // Builder is of higher priority (it contains the cell weights).
  if (rhs.hasBuilder()) {
    replace(rhs.builder(), iomapping,
        onNewCell, onEqualDepth, onGreaterDepth, onRecomputedDetph);
  } else {
    replace(rhs.id(), iomapping, nullptr /* weight provider */,
        onNewCell, onEqualDepth, onGreaterDepth, onRecomputedDetph);
  }
}

void SubnetBuilder::replace(
    const SubnetID rhsID,
    const InOutMapping &iomapping,
    const CellWeightProvider *weightProvider,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onEqualDepth,
    const CellActionCallback *onGreaterDepth,
    const CellActionCallback *onRecomputedDepth) {

  const auto &rhs = Subnet::get(rhsID);
  const auto &rhsEntries = rhs.getEntries();
  replace<Subnet, Array<Entry>, ArrayIterator<Entry>>(
    rhs, rhsEntries, rhsEntries.size() - 1, iomapping,
    [&](ArrayIterator<Entry> iter, EntryID i) {
      return i;
    },
    weightProvider, onNewCell, onEqualDepth, onGreaterDepth, onRecomputedDepth
  );
}

void SubnetBuilder::replace(
    const SubnetBuilder &rhsBuilder,
    const InOutMapping &iomapping,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onEqualDepth,
    const CellActionCallback *onGreaterDepth,
    const CellActionCallback *onRecomputedDepth) {

  const CellWeightProvider weightProvider = [&](EntryID i) -> float {
    return rhsBuilder.getWeight(i);
  };
  replace<SubnetBuilder, SubnetBuilder, EntryIterator>(
    rhsBuilder, rhsBuilder, *(--rhsBuilder.end()), iomapping,
    [&](EntryIterator iter, EntryID i) {
      return *iter;
    },
    &weightProvider, onNewCell, onEqualDepth, onGreaterDepth, onRecomputedDepth
  );
}

//-- FIXME: Implement entryID iterators for Subnet.
template <typename RhsContainer>
void fillMapping(const RhsContainer &rhs,
                 const InOutMapping &iomapping,
                 std::vector<EntryID> &rhsToLhs) {
  assert(false && "Specialization is required");
}

template <>
void fillMapping<Subnet>(const Subnet &rhs,
                         const InOutMapping &iomapping,
                         std::vector<EntryID> &rhsToLhs) {
  assert(rhs.getInNum() == iomapping.getInNum());
  assert(rhs.getOutNum() == iomapping.getOutNum());

  for (uint16_t i = 0; i < iomapping.getInNum(); ++i) {
    rhsToLhs[rhs.getInIdx(i)] = iomapping.getIn(i);
  }
  for (uint16_t i = 0; i < iomapping.getOutNum(); ++i) {
    rhsToLhs[rhs.getOutIdx(i)] = iomapping.getOut(i);
  }
}

template <>
void fillMapping<SubnetBuilder>(const SubnetBuilder &rhs,
                                const InOutMapping &iomapping,
                                std::vector<EntryID> &rhsToLhs) {
  assert(rhs.getInNum() == iomapping.getInNum());
  assert(rhs.getOutNum() == iomapping.getOutNum());

  uint16_t i = 0;
  for (auto it = rhs.begin(); it != rhs.end(); ++it, ++i) {
    if (!rhs.getCell(*it).isIn()) break;
    rhsToLhs[*it] = iomapping.getIn(i);
  }

  uint16_t j = 0;
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
    const EntryID rhsOutEntryID,
    const InOutMapping &iomapping,
    const std::function<EntryID(RhsIt iter, EntryID i)> &getEntryID,
    const CellWeightProvider *weightProvider,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onEqualDepth,
    const CellActionCallback *onGreaterDepth,
    const CellActionCallback *onRecomputedDepth) {

  assert(rhsContainer.getOutNum() == 1);

  std::vector<EntryID> rhsToLhs(rhsContainer.getMaxIdx() + 1);
  fillMapping(rhsContainer, iomapping, rhsToLhs);

  auto rootStrashIt = strash.end();
  const auto lhsRootEntryID = rhsToLhs[rhsOutEntryID];
  const auto &lhsRootCell = getCell(lhsRootEntryID);

  const auto oldLhsRootDepth = getDepth(lhsRootEntryID);
  const auto rhsOutLink = lhsRootCell.isOut() ? Link(rhsOutEntryID, 0, 0) :
      rhsContainer.getLink(rhsOutEntryID, 0);

  // Delete the root entry from the strash map.
  if (rhsToLhs[0] != lhsRootEntryID) {
    destrashEntry(lhsRootEntryID);
  }

  EntryID i = 0;
  for (auto rhsIt = rhsIterable.begin(); rhsIt != rhsIterable.end();
       ++rhsIt, ++i) {
    const auto rhsEntryID = getEntryID(rhsIt, i);
    const auto &rhsCell = rhsContainer.getCell(rhsEntryID);
    const auto rhsCellTypeID = rhsCell.getTypeID();
    assert(rhsCell.arity <= Cell::InPlaceLinks);

    uint32_t prevEntriesN = entries.size();
    uint32_t prevEmptyEntriesN = emptyEntryIDs.size();
    EntryID newEntryID;
    bool isNewEntry = false;

    if (rhsCell.isIn()) {
      continue;
    }
    if (rhsCell.isOut() && !lhsRootCell.isOut()) {
      break;
    }
    LinkList curCellLinks;
    for (const auto &link : rhsContainer.getLinks(rhsEntryID)) {
      curCellLinks.push_back(Link(rhsToLhs[link.idx], link.out, link.inv));
    }

    if (rhsOutLink.idx == rhsEntryID && !rhsOutLink.inv &&
        (lhsRootCell.isOut() ||
        (rootStrashIt = strash.find(StrashKey{rhsCellTypeID, curCellLinks})) ==
         strash.end())) {

      newEntryID = replaceCell(lhsRootEntryID, rhsCellTypeID,
                               curCellLinks, true, onNewCell,
                               onRecomputedDepth).idx;
    } else {
      newEntryID = addCell(rhsCellTypeID, curCellLinks).idx;
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
  if ((rhsOutLink.idx < iomapping.getInNum() &&
       rhsToLhs[0] != lhsRootEntryID) ||
       rhsOutLink.inv || rootStrashIt != strash.end()) {

    LinkList bufLinks;
    CellTypeID bufTID = CELL_TYPE_ID_BUF;
    bufLinks = {Link(rhsToLhs[rhsOutLink.idx], rhsOutLink.out,
                rhsOutLink.inv)};
    replaceCell(lhsRootEntryID, bufTID, bufLinks, true, onNewCell,
                onRecomputedDepth);
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

  const auto rhsOutEntryID = *rhsBuilder.rbegin();
  const CellWeightProvider weightProvider = [&](EntryID i) -> float {
    return rhsBuilder.getWeight(i);
  };
  return evaluateReplace<SubnetBuilder>(rhsBuilder, rhsOutEntryID,
      iomapping, &weightProvider, weightModifier);
}

template <typename RhsContainer>
SubnetBuilder::Effect SubnetBuilder::evaluateReplace(
    const RhsContainer &rhsContainer,
    const EntryID rhsOutEntryID,
    const InOutMapping &iomapping,
    const CellWeightProvider *weightProvider,
    const CellWeightModifier *weightModifier) const {
  assert(rhsContainer.getOutNum() == 1);
  std::unordered_set<EntryID> reusedLhsEntries;
  std::unordered_map<EntryID, uint32_t> entryNewRefcount;
  const auto addEffect = newEntriesEval(rhsContainer, iomapping,
      reusedLhsEntries, entryNewRefcount, weightProvider, weightModifier);
  const auto delEffect = deletedEntriesEval(iomapping.getOut(0),
      reusedLhsEntries, entryNewRefcount, weightModifier);

  return delEffect - addEffect;
}

void SubnetBuilder::replaceWithZero(const EntrySet &entryIDs) {
  const EntryID zeroID = addCell(ZERO).idx;
  mergeCells(MergeMap{{zeroID, entryIDs}});
}

void SubnetBuilder::replaceWithOne(const EntrySet &entryIDs) {
  const EntryID oneID = addCell(ONE).idx;
  mergeCells(MergeMap{{oneID, entryIDs}});
}

void SubnetBuilder::enableFanouts() {
  fanoutsEnabled = true;
  fanouts.reserve(entries.size());
  for (EntryID i = getSubnetBegin(); i != upperBoundID && i != invalidID;
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
    std::unordered_set<EntryID> &reusedLhsEntries,
    std::unordered_map<EntryID, uint32_t> &entryNewRefcount,
    const CellWeightProvider *weightProvider,
    const CellWeightModifier *weightModifier) const {

  const auto &rhsEntries = rhs.getEntries();
  return newEntriesEval<Subnet, Array<Entry>, ArrayIterator<Entry>>(
    rhs, rhsEntries, iomapping,
    [&](ArrayIterator<Entry> iter, EntryID i) {
      return i;
     },
    reusedLhsEntries, entryNewRefcount, weightProvider, weightModifier
  );
}

SubnetBuilder::Effect SubnetBuilder::newEntriesEval(
    const SubnetBuilder &rhsBuilder,
    const InOutMapping &iomapping,
    std::unordered_set<EntryID> &reusedLhsEntries,
    std::unordered_map<EntryID, uint32_t> &entryNewRefcount,
    const CellWeightProvider *weightProvider,
    const CellWeightModifier *weightModifier) const {

  return newEntriesEval<SubnetBuilder, SubnetBuilder, EntryIterator>(
    rhsBuilder, rhsBuilder, iomapping,
    [&](EntryIterator iter, EntryID i) {
      return *iter;
    },
    reusedLhsEntries, entryNewRefcount, weightProvider, weightModifier
  );
}

template <typename RhsContainer>
void SubnetBuilder::incOldLinksRefcnt(
    const RhsContainer &rhsContainer,
    const EntryID rhsEntryID,
    const std::vector<EntryID> &rhsToLhs,
    std::unordered_map<EntryID, uint32_t> &entryNewRefcount) const {
  for (const auto &rhsLink : rhsContainer.getLinks(rhsEntryID)) {
    const auto rhsLinkIdx = rhsLink.idx;
    const auto lhsLinkIdx = rhsToLhs[rhsLinkIdx];
    if (lhsLinkIdx == -1u) {
      continue;
    }
    if (entryNewRefcount.find(lhsLinkIdx) == entryNewRefcount.end()) {
      entryNewRefcount[lhsLinkIdx] = entries[lhsLinkIdx].cell.refcount;
    }
    ++entryNewRefcount[lhsLinkIdx];
  }
}

template <typename RhsContainer, typename RhsIterable, typename RhsIt>
SubnetBuilder::Effect SubnetBuilder::newEntriesEval(
    const RhsContainer &rhsContainer,
    const RhsIterable &rhsIterable,
    const InOutMapping &iomapping,
    const std::function<EntryID(RhsIt iter, EntryID i)> &getEntryID, // FIXME: Add iterator to Subnet
    std::unordered_set<EntryID> &reusedLhsEntries,
    std::unordered_map<EntryID, uint32_t> &entryNewRefcount,
    const CellWeightProvider *weightProvider,
    const CellWeightModifier *weightModifier) const {

  int addedEntriesN = 0;
  float addedWeight = 0.0;

  std::vector<int> virtualDepth(rhsContainer.getMaxIdx() + 1);

  std::vector<EntryID> rhsToLhs(rhsContainer.getMaxIdx() + 1, -1u);
  fillMapping(rhsContainer, iomapping, rhsToLhs);

  EntryID rhsRootEntryID = invalidID;
  EntryID i = 0;
  for (auto rhsIt = rhsIterable.begin(); rhsIt != rhsIterable.end();
       ++rhsIt, ++i) {

    const auto rhsEntryID = getEntryID(rhsIt, i);
    const auto &rhsCell = rhsContainer.getCell(rhsEntryID);
    if (rhsCell.isOut()) {
      const auto lhsEntryID = rhsToLhs[rhsEntryID];
      if (!getCell(lhsEntryID).isOut()) {
        break;
      }
      rhsRootEntryID = rhsEntryID;
      const auto rhsLink = rhsContainer.getLink(rhsEntryID, 0);
      const auto lhsLink = getLink(lhsEntryID, 0);
      if (lhsLink.idx == rhsToLhs[rhsLink.idx] && lhsLink.inv == rhsLink.inv) {
        reusedLhsEntries.insert(lhsEntryID);
      } else {
        ++addedEntriesN;
        addedWeight += weight(rhsEntryID, /*TODO*/ 0, weightProvider, weightModifier);
        incOldLinksRefcnt(rhsContainer, rhsEntryID, rhsToLhs, entryNewRefcount);
      }
      virtualDepth[rhsEntryID] = virtualDepth[rhsLink.idx] + 1;
      break;
    }
    rhsRootEntryID = rhsEntryID;
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
    StrashKey key(rhsCell.getTypeID(), newRhsLinks);
    auto it = strash.end();
    if (isNewElem || (it = strash.find(key)) == strash.end()) {
      ++addedEntriesN;
      addedWeight += weight(rhsEntryID, /*TODO*/ 0, weightProvider, weightModifier);
      incOldLinksRefcnt(rhsContainer, rhsEntryID, rhsToLhs, entryNewRefcount);
      continue;
    }
    rhsToLhs[rhsEntryID] = it->second;
    reusedLhsEntries.insert(it->second);
  }
  return Effect{addedEntriesN, virtualDepth[rhsRootEntryID], addedWeight};
}

SubnetBuilder::Effect SubnetBuilder::deletedEntriesEval(
    const EntryID lhsRootEntryID,
    std::unordered_set<EntryID> &reusedLhsEntries,
    std::unordered_map<EntryID, uint32_t> &entryNewRefcount,
    const CellWeightModifier *weightModifier) const {
  if (reusedLhsEntries.find(lhsRootEntryID) != reusedLhsEntries.end()) {
    Effect effect;
    effect.depth = static_cast<int>(getDepth(lhsRootEntryID));
    return effect;
  }

  int deletedEntriesN = 0;
  float deletedWeight = 0.0;
  deletedEntriesN++;
  deletedWeight += weight(getWeight(lhsRootEntryID), /*TODO*/ 0, weightModifier);
  std::queue<EntryID> entryIDQueue;
  EntryID entryID = lhsRootEntryID;
  entryIDQueue.push(entryID);
  while (!entryIDQueue.empty()) {
    entryID = entryIDQueue.front();
    entryIDQueue.pop();
    const auto &cell = entries[entryID].cell;
    for (uint16_t i = 0; i < cell.arity; ++i) {
      const auto linkIdx = cell.link[i].idx;
      if (entries[linkIdx].cell.isIn()) {
        continue;
      }
      if (entryNewRefcount.find(linkIdx) == entryNewRefcount.end()) {
        entryNewRefcount[linkIdx] = entries[linkIdx].cell.refcount;
      }
      --entryNewRefcount[linkIdx];
      if (!entryNewRefcount[linkIdx]) {
        ++deletedEntriesN;
        deletedWeight += weight(getWeight(linkIdx), /*TODO*/ 0, weightModifier);
        entryIDQueue.push(linkIdx);
      }
    }
  }

  const int oldRootDepth = static_cast<int>(getDepth(lhsRootEntryID));
  return Effect{deletedEntriesN, oldRootDepth, deletedWeight};
}

Subnet::Link &SubnetBuilder::getLinkRef(EntryID entryID, uint16_t j) {
  return const_cast<Subnet::Link&>(getLink(entryID, j));
}

void SubnetBuilder::deleteDepthBounds(EntryID entryID) {
  const uint32_t entryDepth = getDepth(entryID);
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

void SubnetBuilder::addDepthBounds(EntryID entryID) {
  const auto &cell = getCell(entryID);
  const auto typeID = cell.getTypeID();
  const auto curDepth = getDepth(entryID);
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
    auto entryToPlaceAfter = depthBounds[curDepth].second;
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

void SubnetBuilder::addFanout(EntryID sourceID, uint32_t fanoutID) {
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

void SubnetBuilder::delFanout(EntryID sourceID, uint32_t fanoutID) {
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

EntryID SubnetBuilder::allocEntry(bool isBuf) {
  nCell++;
  if (isBuf) nBuf++;

  if (!emptyEntryIDs.empty()) {
    const auto allocatedID = emptyEntryIDs.back();
    emptyEntryIDs.pop_back();
    return allocatedID;
  }

  entries.resize(entries.size() + 1);
  desc.resize(entries.size());

  return entries.size() - 1;
}

EntryID SubnetBuilder::allocEntry(CellTypeID typeID, const LinkList &links) {
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

  const auto idx = (status.first != invalidID) ?
      status.first : allocEntry(typeID == CELL_TYPE_ID_BUF);

  desc[idx].depth = 0;
  desc[idx].session = 0;
  if (desc[idx].simBits) {
    uint64Allocator.deallocate(desc[idx].simBits, desc[idx].simN);
    desc[idx].simBits = nullptr;
    desc[idx].simNext = invalidID;
    desc[idx].simN = 0;
  }

  for (const auto link : links) {
    desc[idx].depth = std::max(getDepth(idx), getDepth(link.idx) + 1);
    auto &cell = getCell(link.idx);
    assert(!cell.isOut());
    addFanout(link.idx, idx);
    cell.incRefCount();
  }
  entries[idx] = Entry(typeID, links);

  addDepthBounds(idx);

  const auto curDepth = getDepth(idx);
  const auto nLinks = links.size();

  if (Cell::InPlaceLinks >= nLinks) {
    return idx;
  }

  const auto saveNextEntryID = getNext(idx);
  auto prevEntryID = idx;

  for (uint16_t i = Cell::InPlaceLinks; i < nLinks; i += Cell::InEntryLinks) {
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

void SubnetBuilder::deallocEntry(EntryID entryID) {
  const auto &cell = getCell(entryID);
  const bool isBuf = cell.isBuf();
  assert(!cell.refcount);

  destrashEntry(entryID);

  // Updating depth bounds.
  deleteDepthBounds(entryID);
  desc[entryID].depth = invalidID;
  emptyEntryIDs.push_back(entryID);

  nCell--;
  if (isBuf) nBuf--;
}

void SubnetBuilder::mergeCells(const MergeMap &entryIDs) {
  uint32_t refcount = 0;

  std::unordered_map<EntryID, EntryID> mergeTo;
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
    for (uint16_t j = 0; j < target.arity; ++j) {
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

EntryID SubnetBuilder::getNext(const EntryID entryID) const {
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

EntryID SubnetBuilder::getPrev(const EntryID entryID) const {
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

void SubnetBuilder::setOrder(const EntryID firstID, const EntryID secondID) {
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

void SubnetBuilder::placeAfter(EntryID entryID, EntryID pivotEntryID) {
  assert(pivotEntryID != upperBoundID);
  setOrder(entryID, getNext(pivotEntryID));
  setOrder(pivotEntryID, entryID);
}

void SubnetBuilder::placeBefore(EntryID entryID, EntryID pivotEntryID) {
  assert(pivotEntryID != lowerBoundID);
  setOrder(getPrev(pivotEntryID), entryID);
  setOrder(entryID, pivotEntryID);
}

void SubnetBuilder::recomputeFanoutDepths(
    EntryID rootEntryID,
    EntryID oldRootNextEntryID,
    const CellActionCallback *onRecomputedDepth) {
  const auto &rootCell = getCell(rootEntryID);
  if (!rootCell.refcount) {
    return;
  }

  std::unordered_set<EntryID> toRecompute;
  toRecompute.insert(rootEntryID);
  auto toRecomputeN = rootCell.refcount;
  auto curEntryID = oldRootNextEntryID;
  while (toRecomputeN) {
    if (toRecompute.find(curEntryID) != toRecompute.end()) {
      curEntryID = getNext(curEntryID);
      continue;
    }
    const auto &curCell = getCell(curEntryID);
    uint32_t newDepth = 0;
    const auto curDepth = getDepth(curEntryID);
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
    auto nextEntryID = getNext(curEntryID);
    // Changing topological order
    deleteDepthBounds(curEntryID);
    desc[curEntryID].depth = newDepth;
    if (onRecomputedDepth) {
      (*onRecomputedDepth)(curEntryID);
    }
    addDepthBounds(curEntryID);
    curEntryID = nextEntryID;
  }
}

void SubnetBuilder::relinkCell(EntryID entryID, const LinkList &newLinks) {
  auto &cell = getCell(entryID);
  assert(cell.arity == newLinks.size());

  for (uint16_t j = 0; j < cell.arity; ++j) {
    auto &link = getLinkRef(entryID, j);
    delFanout(link.idx, entryID);
    link = newLinks[j];
    addFanout(link.idx, entryID);
  }
}

void SubnetBuilder::deleteCell(EntryID entryID) {
  std::queue<EntryID> queue;
  queue.push(entryID);

  do {
    const auto currentID = queue.front();
    queue.pop();

    auto &cell = getCell(currentID);
    assert(cell.arity <= Cell::InPlaceLinks);
    deallocEntry(currentID);

    for (uint16_t j = 0; j < cell.arity; ++j) {
      const EntryID inputID = cell.link[j].idx;

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
    EntryID entryID, CellTypeID typeID, const LinkList &links,
    bool delZeroRefcount,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onRecomputedDepth) {

  auto &cell = getCell(entryID);
  const auto &cellTypeID = cell.getTypeID();
  assert(StrashKey::isEnabled(typeID, links) ||
         (typeID == CELL_TYPE_ID_OUT && cellTypeID == CELL_TYPE_ID_OUT));

  destrashEntry(entryID);

  if (cell.isBuf()) nBuf--;
  if (typeID == CELL_TYPE_ID_BUF) nBuf++;

  const auto oldRootNext = getNext(entryID);
  const auto oldRefcount = cell.refcount;
  const auto oldLinks = getLinks(entryID);
  const auto oldDepth = getDepth(entryID);
  uint32_t newDepth = 0;

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
  cell.refcount = oldRefcount;

  bool equalRoots;
  if (StrashKey::isEnabled(typeID, links)) {
    const auto newRootStrKey = StrashKey(typeID, links);
    const auto oldRootStrKey = StrashKey(cellTypeID, oldLinks);
    auto it = strash.find({newRootStrKey});
    if (it == strash.end()) {
      strash.insert({StrashKey(cell), entryID});
    }
    equalRoots = newRootStrKey == oldRootStrKey;
  } else {
    equalRoots = links[0] == oldLinks[0];
  }

  if (!equalRoots) {
    desc[entryID].session = 0;
    if (desc[entryID].simBits) {
      uint64Allocator.deallocate(desc[entryID].simBits, desc[entryID].simN);
      desc[entryID].simBits = nullptr;
      desc[entryID].simNext = invalidID;
      desc[entryID].simN = 0;
    }
  }
  if (oldDepth != newDepth) {
    deleteDepthBounds(entryID);
    desc[entryID].depth = newDepth;
    if (onNewCell) {
      (*onNewCell)(entryID);
    }
    addDepthBounds(entryID);
    recomputeFanoutDepths(entryID, oldRootNext, onRecomputedDepth);
  } else if (onNewCell) {
    (*onNewCell)(entryID);
  }

  return Link(entryID);
}

bool SubnetBuilder::checkInputsOrder() const {
  for (uint32_t i = 0; i < nIn; ++i) {
    const auto &cell = getCell(i);

    if (!cell.isIn()) {
      return false;
    }

    i += cell.more;
  }
  return true;
}

bool SubnetBuilder::checkOutputsOrder() const {
  for (uint32_t i = entries.size() - nOut; i < entries.size(); ++i) {
    const auto &cell = getCell(i);

    if (!cell.isOut()) {
      return false;
    }

    i += cell.more;
  }
  return true;
}

void SubnetBuilder::rearrangeEntries(
    std::vector<EntryID> &entryMapping,
    const bool deleteBufs) {
  std::vector<Entry> newEntries;
  std::vector<EntryDescriptor> newDesc;
  std::vector<EntryID> saveMapping;
  newEntries.reserve(entries.size());
  newDesc.reserve(desc.size());
  if (!entryMapping.empty()) {
    saveMapping.resize(entryMapping.size(), invalidID);
  }

  std::unordered_map<EntryID, std::pair<EntryID, bool /* inv */>> relinkMapping;
  relinkMapping.reserve(entries.size());
  bool outVisited = false;
  uint32_t lastCellDepth = invalidID;
  uint16_t isLink = 0;
  for(uint32_t i = getSubnetBegin(); i != upperBoundID; i = getNext(i)) {
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
      nCell--;
      nBuf--;
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
    const uint16_t nFanin = cell.arity;
    isLink += std::max(nFanin, Cell::InPlaceLinks) - Cell::InPlaceLinks;
    EntryDescriptor newCellDesc;
    newCellDesc.weight = getWeight(i);
    newCellDesc.depth = 0;
    const auto &oldLinks = getLinks(i);
    LinkList newLinks;
    for (uint16_t j = 0; j < oldLinks.size(); ++j) {
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

std::pair<EntryID, bool> SubnetBuilder::strashEntry(
    CellTypeID typeID, const LinkList &links) {
  if (StrashKey::isEnabled(typeID, links)) {
    const StrashKey key(typeID, links);
    const auto i = strash.find(key);

    if (i != strash.end()) {
      return {i->second, false /* old */};
    }

    const auto idx = allocEntry(typeID == CELL_TYPE_ID_BUF);
    strash.insert({key, idx});

    return {idx, true /* new */};
  }

  return {invalidID, false};
}

void SubnetBuilder::destrashEntry(EntryID entryID) {
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
