//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/premapper/cell_premapper.h"

namespace eda::gate::premapper {

using Link             = model::Subnet::Link;
using LinkList         = model::Subnet::LinkList;
using SubnetBuilder    = model::SubnetBuilder;
using SubnetBuilderPtr = std::shared_ptr<SubnetBuilder>;

SubnetBuilderPtr CellPremapper::map(const SubnetBuilderPtr &builder) const {
  const auto subnetID = builder->make();
  auto premapped = std::make_shared<SubnetBuilder>();

  CellIdMap oldToNew;
  const auto &oldSubnet = Subnet::get(subnetID);
  const auto &entries = oldSubnet.getEntries();

  for (uint32_t i = 0; i < oldSubnet.size(); ++i) {
    const auto &cell = entries[i].cell;
    const auto symbol = cell.getSymbol();

    size_t n0 = 0;
    size_t n1 = 0;

    const LinkList links = getNewLinks(oldToNew, i, oldSubnet,
                                       n0, n1, *premapped);

    bool inv = false;
    Link link = mapCell(symbol, links, inv, n0, n1, *premapped);
    link.inv = link.inv != inv;
    oldToNew[i] = link;

    i += cell.more;
  }

  return premapped;
}

Link CellPremapper::mapCell(CellSymbol symbol, const LinkList &links,
                            bool &inv, size_t n0, size_t n1,
                            SubnetBuilder &builder) const {

  switch (symbol) {
    case CellSymbol::IN   : return mapIn (                    builder);
    case CellSymbol::OUT  : return mapOut(links,              builder);
    case CellSymbol::ZERO : return mapVal(       false,       builder);
    case CellSymbol::ONE  : return mapVal(       true,        builder);
    case CellSymbol::BUF  : return mapBuf(links,              builder);
    case CellSymbol::AND  : return mapAnd(links, inv, n0, n1, builder);
    case CellSymbol::OR   : return mapOr (links, inv, n0, n1, builder);
    case CellSymbol::XOR  : return mapXor(links, inv, n0, n1, builder);
    case CellSymbol::MAJ  : return mapMaj(links, inv, n0, n1, builder);
    default: assert(false && "Unknown gate");
  }
}

LinkList CellPremapper::getNewLinks(const CellIdMap &oldToNew, uint32_t idx,
                                    const Subnet &oldSubnet,
                                    size_t &n0, size_t &n1,
                                    SubnetBuilder &builder) const {

  LinkList links = oldSubnet.getLinks(idx);
  for (auto &link : links) {
    const uint32_t oldId = link.idx;

    const auto search = oldToNew.find(oldId);
    assert(search != oldToNew.end() && "Old cell ID not found");

    const Link cellLink = search->second;
    link.idx = cellLink.idx;
    link.inv = cellLink.inv != link.inv;

    const auto &cell = builder.getCell(link.idx);

    bool isZero = (cell.isZero() && !link.inv) || (cell.isOne() &&  link.inv);
    bool isOne  = (cell.isZero() &&  link.inv) || (cell.isOne() && !link.inv);

    if (isZero) n0++;
    if (isOne ) n1++;
  }

  return links;
}

Link CellPremapper::mapIn(SubnetBuilder &builder) const {
  return builder.addInput();
}

Link CellPremapper::mapOut(const LinkList &links,
                           SubnetBuilder &builder) const {

  assert(links.size() == 1 && "Only single input is allowed in OUT cell");
  Link link = links.front();

  if (link.inv) {
    link = builder.addCell(CellSymbol::BUF, links);
  }

  return builder.addOutput(link);
}

Link CellPremapper::mapVal(bool val, SubnetBuilder &builder) const {
  if (val) {
    return builder.addCell(CellSymbol::ONE);
  }
  return builder.addCell(CellSymbol::ZERO);
}

Link CellPremapper::mapBuf(const LinkList &links,
                           SubnetBuilder &builder) const {

  assert(links.size() == 1 && "Only single input is allowed in BUF cell");
  return builder.addCell(CellSymbol::BUF, links);
}

Link CellPremapper::mapAnd(const LinkList &links, bool &inv,
                           size_t n0, size_t n1,
                           SubnetBuilder &builder) const {

  const size_t linksSize = links.size();
  // Consider simple cases.
  if (n0 > 0) {
    return mapVal(false, builder);
  }
  if (n1 == linksSize) {
    return mapVal(true, builder);
  }
  if (linksSize == 1) {
    return mapBuf(links, builder);
  }
  // Erase links from constant cells.
  LinkList linkList(links);
  size_t i = linksSize;
  do {
    --i;
    const auto &cell = builder.getCell(linkList[i].idx);
    if (cell.isOne() || cell.isZero()) {
      linkList.erase(linkList.begin() + i);
    }
  } while (i);

  // Consider simple cases for one and two inputs only.
  if (linkList.size() == 1) {
    return mapBuf(linkList, builder);
  }
  if (linkList[0].idx == linkList[1].idx) {
    if ((linkList.size() == 2) && (linkList[0].inv == linkList[1].inv)) {
      return mapBuf(LinkList{linkList[0]}, builder);
    }
    if (linkList[0].inv != linkList[1].inv) {
      return mapVal(false, builder);
    }
    linkList.erase(linkList.begin() + 1);
  }

  return mapAnd(linkList, inv, builder);
}

Link CellPremapper::mapOr(const LinkList &links, bool &inv,
                          size_t n0, size_t n1,
                          SubnetBuilder &builder) const {

  const size_t linksSize = links.size();
  // Consider simple cases.
  if (n1 > 0) {
    return mapVal(true, builder);
  }
  if (n0 == linksSize) {
    return mapVal(false, builder);
  }
  if (linksSize == 1) {
    return mapBuf(links, builder);
  }
  // Erase links from constants.
  LinkList linkList(links);
  size_t i = linksSize;
  do {
    --i;
    const auto &cell = builder.getCell(linkList[i].idx);
    if (cell.isOne() || cell.isZero()) {
      linkList.erase(linkList.begin() + i);
    }
  } while (i);

  // Consider simple cases for one and two inputs only.
  if (linkList.size() == 1) {
    return mapBuf(linkList, builder);
  }
  if (linkList[0].idx == linkList[1].idx) {
    if ((linkList.size() == 2) && (linkList[0].inv == linkList[1].inv)) {
      return mapBuf(LinkList{linkList[0]}, builder);
    }
    if (linkList[0].inv != linkList[1].inv) {
      return mapVal(true, builder);
    }
    linkList.erase(linkList.begin() + 1);
  }

  return mapOr(linkList, inv, builder);
}

Link CellPremapper::mapXor(const LinkList &links, bool &inv,
                           size_t n0, size_t n1,
                           SubnetBuilder &builder) const {

  const size_t linksSize = links.size();
  // Consider simple cases.
  if (n0 == linksSize) {
    return mapVal(false, builder);
  }
  if (n1 + n0 == linksSize) {
    return mapVal(n1 & 1u, builder);
  }
  if (linksSize == 1) {
    return mapBuf(links, builder);
  }
  // Erase links from constant cells.
  LinkList linkList(links);
  size_t i = linksSize;
  do {
    --i;

    const auto &link = linkList[i];
    const auto &cell = builder.getCell(link.idx);

    if (cell.isOne() || cell.isZero()) {
      bool one = (cell.isOne() && !link.inv) || (cell.isZero() && link.inv);
      inv = inv != one;
      linkList.erase(linkList.begin() + i);
    }
  } while (i);

  // Consider simple cases for one and two inputs only.
  if (linkList.size() == 1) {
    return mapBuf(linkList, builder);
  }
  if ((linkList.size() == 2) && (linkList[0].idx == linkList[1].idx)) {
    if (linkList[0].inv == linkList[1].inv) {
      return mapVal(false, builder);
    }
    return mapVal(true, builder);
  }

  return mapXor(linkList, inv, builder);
}

Link CellPremapper::mapMaj(const LinkList &links, bool &inv,
                           size_t n0, size_t n1,
                           SubnetBuilder &builder) const {

  size_t linksSize = links.size();
  if (linksSize == 1) {
    return mapBuf(links, builder);
  }

  assert((linksSize % 2 == 1) && (linksSize >= 3) && "Invalid number of links");

  if (n0 > linksSize / 2) {
    return mapVal(false, builder);
  }
  if (n1 > linksSize / 2) {
    return mapVal(true, builder);
  }
  if (!n0 && !n1) {
    return mapMaj(links, inv, builder);
  }

  bool n0More = n0 > n1;
  size_t nRemained = n0More ? n0 - n1 : n1 - n0;
  LinkList linkList(links);
  // Erase redundant constants.
  size_t i = linksSize;
  do {
    --i;
    auto link = linkList[i];
    const auto &cell = builder.getCell(link.idx);

    bool zero = (cell.isZero() && !link.inv) || (cell.isOne() && link.inv);
    if ((zero && !n0More) || (zero && n0More && (n0 != nRemained))) {
      linkList.erase(linkList.begin() + i);
      assert(n0);
      --n0;
      continue;
    }

    bool one = (cell.isOne() && !link.inv) || (cell.isZero() && link.inv);
    if ((one && n0More) || (one && !n0More && (n1 != nRemained))) {
      linkList.erase(linkList.begin() + i);
      assert(n1);
      --n1;
    }
  } while (i);

  const size_t linkListSize = linkList.size();
  if (linkListSize == 1) {
    return mapBuf(linkList, builder);
  }
  // Check whether MAJ cell implements AND, OR cells.
  if ((linkListSize / 2 == n0) || (linkListSize / 2 == n1)) {
    i = linkListSize;
    do {
      --i;
      const auto &cell = builder.getCell(linkList[i].idx);
      if (cell.isZero() || cell.isOne()) {
        linkList.erase(linkList.begin() + i);
      }
    } while (i);

    if (n0) return mapAnd(linkList, inv, builder);
    if (n1) return mapOr (linkList, inv, builder);
  }

  return mapMaj(linkList, inv, builder);
}

} // namespace eda::gate::premapper
