//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <gate/premapper/premapper.h>

namespace eda::gate::premapper {

using Link          = Premapper::Link;
using LinkList      = Premapper::LinkList;
using SubnetBuilder = Premapper::SubnetBuilder;

optimizer::SubnetBuilderPtr Premapper::make(const SubnetID subnetID) const {
  auto builder = std::make_shared<SubnetBuilder>();

  CellIdMap oldToNew;
  const auto &oldSubnet = Subnet::get(subnetID);
  const auto &entries = oldSubnet.getEntries();

  for (uint32_t i = 0; i < oldSubnet.size(); ++i) {
    const auto &cell = entries[i].cell;
    const auto symbol = cell.getSymbol();

    size_t n0 = 0;
    size_t n1 = 0;

    const LinkList links = getNewLinks(oldToNew, i, oldSubnet,
                                       n0, n1, *builder);

    bool inv = false;
    Link link = mapCell(symbol, links, inv, n0, n1, *builder);
    link.inv = link.inv != inv;
    oldToNew[i] = link;

    i += cell.more;
  }

  return builder;
}

Link Premapper::mapCell(CellSymbol symbol, const LinkList &links, bool &inv,
                        size_t n0, size_t n1, SubnetBuilder &builder) const {

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

LinkList Premapper::getNewLinks(const CellIdMap &oldToNew, uint32_t idx,
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

Link Premapper::mapIn(SubnetBuilder &builder) const {
  return builder.addInput();
}

Link Premapper::mapOut(const LinkList &links, SubnetBuilder &builder) const {
  assert(links.size() == 1 && "Only single input is allowed in OUT cell");
  Link link = links.front();

  if (link.inv) {
    link = builder.addCell(CellSymbol::BUF, links);
  }

  return builder.addOutput(link);
}

Link Premapper::mapVal(bool val, SubnetBuilder &builder) const {
  if (val) {
    return builder.addCell(CellSymbol::ONE);
  }
  return builder.addCell(CellSymbol::ZERO);
}

Link Premapper::mapBuf(const LinkList &links, SubnetBuilder &builder) const {
  assert(links.size() == 1 && "Only single input is allowed in BUF cell");
  return builder.addCell(CellSymbol::BUF, links);
}

Link Premapper::mapAnd(const LinkList &links, bool &inv, size_t n0, size_t n1,
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

Link Premapper::mapOr(const LinkList &links, bool &inv, size_t n0, size_t n1,
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

Link Premapper::mapXor(const LinkList &links, bool &inv, size_t n0, size_t n1,
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

Link Premapper::mapMaj(const LinkList &links, bool &inv, size_t n0, size_t n1,
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
  return mapMaj(links, inv, builder);
}

} // namespace eda::gate::premapper
