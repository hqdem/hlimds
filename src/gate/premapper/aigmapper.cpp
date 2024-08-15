//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/premapper/aigmapper.h"

namespace eda::gate::premapper {

using Link          = AigMapper::Link;
using LinkList      = AigMapper::LinkList;
using SubnetBuilder = AigMapper::SubnetBuilder;

//===----------------------------------------------------------------------===//
// General functions
//===----------------------------------------------------------------------===//

optimizer::SubnetBuilderPtr AigMapper::make(const SubnetID subnetID) const {
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

Link AigMapper::mapCell(CellSymbol symbol, const LinkList &links, bool &inv,
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

LinkList AigMapper::getNewLinks(const CellIdMap &oldToNew, uint32_t idx,
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

//===----------------------------------------------------------------------===//
// IN
//===----------------------------------------------------------------------===//

Link AigMapper::mapIn(SubnetBuilder &builder) const {
  return builder.addInput();
}

//===----------------------------------------------------------------------===//
// OUT
//===----------------------------------------------------------------------===//

Link AigMapper::mapOut(const LinkList &links, SubnetBuilder &builder) const {
  assert(links.size() == 1 && "Only single input is allowed in OUT cell");
  Link link = links.front();

  if (link.inv) {
    link = builder.addCell(CellSymbol::BUF, links);
  }

  return builder.addOutput(link);
}

//===----------------------------------------------------------------------===//
// VALUE (ZERO, ONE)
//===----------------------------------------------------------------------===//

Link AigMapper::mapVal(bool val, SubnetBuilder &builder) const {
  if (val) {
    return builder.addCell(CellSymbol::ONE);
  }
  return builder.addCell(CellSymbol::ZERO);
}

//===----------------------------------------------------------------------===//
// BUF
//===----------------------------------------------------------------------===//

Link AigMapper::mapBuf(const LinkList &links, SubnetBuilder &builder) const {
  assert(links.size() == 1 && "Only single input is allowed in BUF cell");
  return builder.addCell(CellSymbol::BUF, links);
}

//===----------------------------------------------------------------------===//
// AND
//===----------------------------------------------------------------------===//

Link AigMapper::mapAnd(const LinkList &links, bool &inv, size_t n0, size_t n1,
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

Link AigMapper::mapAnd(const LinkList &links, bool &inv,
                       SubnetBuilder &builder) const {

  return builder.addCellTree(CellSymbol::AND, links, 2);
}

//===----------------------------------------------------------------------===//
// OR
//===----------------------------------------------------------------------===//

Link AigMapper::mapOr(const LinkList &links, bool &inv, size_t n0, size_t n1,
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

Link AigMapper::mapOr(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const {

  LinkList linkList(links);
  // OR(x[1],...,x[n]) = ~(AND(~x[1],...,~x[n])).
  inv = !inv;
  for (auto &link : linkList) {
    link.inv = !link.inv;
  }
  return mapAnd(linkList, inv, builder);
}

//===----------------------------------------------------------------------===//
// XOR
//===----------------------------------------------------------------------===//

Link AigMapper::mapXor(const LinkList &links, bool &inv, size_t n0, size_t n1,
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

Link AigMapper::mapXor(const LinkList &links, bool &inv,
                       SubnetBuilder &builder) const {

  LinkList linkList(links);
  linkList.reserve(2 * links.size() - 1);

  size_t l = 0;
  size_t r = 1;
  while (r < linkList.size()) {
    // XOR(x,y) = AND(~AND(x,y),~AND(~x,~y)).
    const Link x = linkList[l];
    const Link y = linkList[r];

    const Link nx(linkList[l].idx, !linkList[l].inv);
    const Link ny(linkList[r].idx, !linkList[r].inv);

    Link l1 = builder.addCell(CellSymbol::AND, x, y);
    Link l2 = builder.addCell(CellSymbol::AND, nx, ny);

    l1.inv = true;
    l2.inv = true;

    linkList.emplace_back(builder.addCell(CellSymbol::AND, l1, l2));

    l += 2;
    r += 2;
  }

  return linkList[l];
}

//===----------------------------------------------------------------------===//
// MAJ
//===----------------------------------------------------------------------===//

Link AigMapper::mapMaj(const LinkList &links, bool &inv, size_t n0, size_t n1,
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

Link AigMapper::mapMaj(const LinkList &links, bool &inv,
                       SubnetBuilder &builder) const {

  size_t linksSize = links.size();

  if (linksSize == 3) {
    return addMaj3(links, inv, builder);
  }
  if (linksSize == 5) {
    return addMaj5(links, inv, builder);
  }

  assert(false && "Unsupported number of links in MAJ cell");
  return Link(0);
}

Link AigMapper::addMaj3(const LinkList &links, bool &inv,
                        SubnetBuilder &builder) const {

  LinkList linkList(links);
  // MAJ(x,y,z)=OR(AND(x,y), AND(y,z), AND(z,x))
  linkList[0] = builder.addCell(CellSymbol::AND, links[0], links[1]);
  linkList[1] = builder.addCell(CellSymbol::AND, links[1], links[2]);
  linkList[2] = builder.addCell(CellSymbol::AND, links[2], links[0]);

  return mapOr(linkList, inv, builder);
}

Link AigMapper::addMaj5(const LinkList &links, bool &inv,
                        SubnetBuilder &builder) const {

  assert(links.size() == 5 && "Invalid number of links for MAJ5 element");
  // <xyztu> = <<xyz>t<<xyu>uz>>
  LinkList tmp(links.begin(), links.begin() + 3);
  const Link xyzLink = addMaj3(tmp, inv, builder);

  tmp[0] = links[0];
  tmp[1] = links[1];
  tmp[2] = links[4];
  const Link xyuLink = addMaj3(tmp, inv, builder);
  // <<xyu>uz>
  tmp[0] = links[2];
  tmp[1] = ~xyuLink;
  tmp[2] = links[4];
  const Link muzLink = addMaj3(tmp, inv, builder);

  tmp[0] = ~xyzLink;
  tmp[1] = links[3];
  tmp[2] = ~muzLink;
  inv = false;
  return addMaj3(tmp, inv, builder);
}

} // namespace eda::gate::premapper
