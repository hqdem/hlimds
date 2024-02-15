//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"
#include "gate/model2/printer/printer.h"

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet
//===----------------------------------------------------------------------===//

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

Subnet::Link SubnetBuilder::addCell(CellTypeID typeID, const LinkList &links) {
  const bool isPositive = !CellType::get(typeID).isNegative();
  assert(isPositive && "Negative cells are not allowed");

  const bool in  = (typeID == CELL_TYPE_ID_IN);
  const bool out = (typeID == CELL_TYPE_ID_OUT);

  assert((!in || entries.size() == nIn)
      && "Input cells after non-input cells are not allowed");

  const auto idx = allocEntry();

  for (const auto link : links) {
    auto &cell = entries[link.idx].cell;
    assert(!cell.isOut());

    // Update reference counts.
    assert(cell.refcount < Subnet::Cell::MaxRefCount);
    cell.refcount++;
  }

  entries[idx] = Subnet::Entry(typeID, links);
  if (subnetEnd != boundEntryID) {
    setOrder(subnetEnd, idx);
  }

  if (in)  nIn++;
  if (out) nOut++;

  const auto InPlaceLinks = Subnet::Cell::InPlaceLinks;
  const auto InEntryLinks = Subnet::Cell::InEntryLinks;
  for (size_t i = InPlaceLinks; i < links.size(); i += InEntryLinks) {
    entries.emplace_back(links, i);
  }

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
    const SubnetID subnetID, const LinkList &links) {

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
        newLink = links[newLink.idx];
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
    const SubnetID subnetID, const LinkList &links) {
  const auto &subnet = Subnet::get(subnetID);
  assert(subnet.getOutNum() == 1);

  return addSubnet(subnetID, links).front();
}

void SubnetBuilder::replace(
    const SubnetID rhsID,
    std::unordered_map<size_t, size_t> &rhsToLhs) {

  if (subnetEnd == boundEntryID) {
    subnetEnd = entries.size() - 1;
  }
  const Subnet &rhs = Subnet::get(rhsID);
  assert(rhs.getOutNum() == 1);
  size_t prevNewCellID = (size_t)-1;
  const auto &rhsEntries = rhs.getEntries();

  for (size_t i = 0; i < rhsEntries.size() - 1; ++i) {
    const Subnet::Cell &cell = rhsEntries[i].cell;
    assert(cell.arity <= Subnet::Cell::InPlaceLinks);
    if (cell.isIn()) {
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
    if (rhs.getOut(0).idx == i) {
      newEntryID = replaceCell(rhsToLhs[rhsEntries.size() - 1],
                               cell.getTypeID(), curCellLinks).idx;
      isNewElem = true;
    } else {
      const size_t curSubnetEnd = subnetEnd;
      newEntryID = addCell(cell.getTypeID(), curCellLinks).idx;
      subnetEnd = curSubnetEnd;
      if (prevEntriesN + 1 == entries.size() ||
          prevEmptyEntriesN == emptyEntryIDs.size() + 1) {

        isNewElem = true;
        rhsToLhs[i] = newEntryID;
      }
    }

    if (!isNewElem) {
      continue;
    }
    if (prevNewCellID == (size_t)-1) {
      setOrder(getPrev(rhsToLhs[rhsEntries.size() - 1]), newEntryID);
    } else {
      setOrder(prevNewCellID, newEntryID);
    }
    prevNewCellID = newEntryID;
  }
}

void SubnetBuilder::sortEntries() {
  std::vector<Subnet::Entry> newEntries;
  std::unordered_map<size_t, size_t> relinkMapping;
  size_t curID = 0;
  do {
    relinkMapping[curID] = newEntries.size();
    LinkList links;
    const auto cell = entries[curID].cell;
    for (size_t i = 0; i < cell.arity; ++i) {
      const auto &curLink = cell.link[i];
      if (relinkMapping.find(curLink.idx) != relinkMapping.end()) {
        links.push_back(Link(relinkMapping[curLink.idx], curLink.out,
                             curLink.inv));
      } else {
        links.push_back(Link(curLink.idx, curLink.out, curLink.inv));
      }
    }
    relinkCell(curID, links);

    newEntries.push_back(entries[curID]);
    curID = getNext(curID);
  } while (curID != boundEntryID);

  entries = std::move(newEntries);
  clearContext();
}

void SubnetBuilder::deleteCell(const size_t entryID) {
  auto &cell = entries[entryID].cell;
  assert(cell.arity <= Subnet::Cell::InPlaceLinks);

  deallocEntry(entryID);
  for (size_t j = 0; j < cell.arity; ++j) {
    const size_t inputEntryID = cell.link[j].idx;
    auto &inputCell = entries[inputEntryID].cell;
    inputCell.refcount--;
    if (!inputCell.refcount) {
      deleteCell(inputEntryID);
    }
  }
}

Subnet::Link SubnetBuilder::replaceCell(
    const size_t entryID,
    const CellTypeID typeID,
    const LinkList &links) {
  assert(links.size() <= Subnet::Cell::InPlaceLinks);

  auto &cell = entries[entryID].cell;
  for (const auto &link : links) {
    entries[link.idx].cell.refcount++;
  }
  Subnet::Entry newCellEntry(typeID, links);
  for (size_t j = 0; j < cell.arity; ++j) {
    const size_t inputEntryID = cell.link[j].idx;
    auto &inputCell = entries[inputEntryID].cell;
    inputCell.refcount--;
    if (!inputCell.refcount) {
      deleteCell(inputEntryID);
    }
  }
  newCellEntry.cell.refcount = cell.refcount;
  entries[entryID] = std::move(newCellEntry);
  return Link(entryID);
}

} // namespace eda::gate::model
