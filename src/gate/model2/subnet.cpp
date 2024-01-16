//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/subnet.h"

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Subnet
//===----------------------------------------------------------------------===//

std::pair<uint32_t, uint32_t> Subnet::getPathLength() const {
  uint32_t minLength = nCell, maxLength = 0;
  std::vector<uint32_t> min(nCell), max(nCell);

  const auto &entries = getEntries();
  for (size_t i = 0; i < nCell; ++i) {
    const auto &cell = entries[i].cell;

    if (cell.isIn()) {
      min[i] = max[i] = 0;
    } else {
      min[i] = nCell; max[i] = 0;

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
  const auto &entries = subnet.getEntries();
  for (size_t i = 0; i < subnet.size(); ++i) {
    const auto &cell = entries[i].cell;
    const auto &type = cell.getType();

    out << i << " <= " << type.getName();
    out << "(";

    bool comma = false;
    for (size_t j = 0; j < cell.arity; ++j) {
      if (comma) out << ", ";

      auto link = subnet.getLink(i, j);

      if (link.inv) out << "~";
      out << link.idx << "." << link.out;

      comma = true;
    }

    out << ");" << std::endl;
    i += cell.more;
  }

  return out;
}

//===----------------------------------------------------------------------===//
// Subnet Builder
//===----------------------------------------------------------------------===//

size_t SubnetBuilder::addCell(CellTypeID typeID, const LinkList &links) {
  bool isPositive = !CellType::get(typeID).isNegative();
  assert(isPositive && "Only positive cells are allowed in a subnet");

  const bool in  = (typeID == CELL_TYPE_ID_IN);
  const bool out = (typeID == CELL_TYPE_ID_OUT);
  const auto idx = entries.size();

  for (const auto link : links) {
    auto &cell = entries[link.idx].cell;
    assert(!cell.isOut());

    // Update reference counts.
    assert(cell.refcount < Subnet::Cell::MaxRefCount);
    cell.refcount++;
  }

  entries.emplace_back(typeID, links);
  if (in)  nIn++;
  if (out) nOut++;

  const auto InPlaceLinks = Subnet::Cell::InPlaceLinks;
  const auto InEntryLinks = Subnet::Cell::InEntryLinks;
  for (size_t i = InPlaceLinks; i < links.size(); i += InEntryLinks) {
    entries.emplace_back(links, i);
  }

  return idx;
}

size_t SubnetBuilder::addCellTree(
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

  return linkList.back().idx;
}

SubnetBuilder::LinkList SubnetBuilder::addSubnet(
    const SubnetID subnetID, const LinkList &links) {
  
  const auto offset = entries.size();

  const auto &subnet = Subnet::get(subnetID);
  const auto &entries = subnet.getEntries();

  LinkList outs;
  outs.reserve(subnet.getOutNum());

  for (size_t i = subnet.getInNum(); i < entries.size(); ++i) {
    auto newLinks = subnet.getLinks(i);

    for (size_t j = 0; j < newLinks.size(); ++j) {
      auto &newLink = newLinks[j];
      if (newLink.idx < nIn) {
        newLink = links[newLink.idx];
      } else {
        newLink.idx += offset;
      }
    }

    const auto &cell = entries[i].cell;
    i += cell.more;

    if (cell.isOut()) {
      outs.push_back(cell.link[0]);
    }
  }

  return outs; 
}

SubnetBuilder::Link SubnetBuilder::addSingleOutputSubnet(
    const SubnetID subnetID, const LinkList &links) {
  const auto &subnet = Subnet::get(subnetID);
  assert(subnet.getOutNum() == 1);

  return addSubnet(subnetID, links).front();
}

} // namespace eda::gate::model
