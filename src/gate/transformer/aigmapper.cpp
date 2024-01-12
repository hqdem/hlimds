//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <gate/transformer/aigmapper.h>

namespace eda::gate::transformer {

using SubnetID = AigMapper::SubnetID;
using LinkList = AigMapper::LinkList;

SubnetID AigMapper::transform(SubnetID id) {
  Builder builder;
  CellIdMap oldToNew;
  InvCells toInvert;
  const auto &oldSubnet = Subnet::get(id);
  const auto cells = oldSubnet.getEntries();
  for (size_t i = 0; i < oldSubnet.size(); ++i) {
    const auto &cell = cells[i].cell;
    const auto symbol = cell.getSymbol();

    size_t n0 = 0;
    size_t n1 = 0;
    LinkList links = getNewLinks(oldToNew, i, oldSubnet, cells,
                                 n0, n1, toInvert);

    bool inv = false;
    const size_t cellId = mapCell(symbol, links, inv, n0, n1, builder);
    oldToNew[i] = cellId;
    if (inv) {
      toInvert.insert(cellId);
    }

    i += cell.more;
  }

  return builder.make();
}

size_t AigMapper::mapCell(CellSymbol symbol, LinkList &links, bool &inv,
                          size_t n0, size_t n1, Builder &builder) const {

  switch (symbol) {
    case CellSymbol::IN   : return mapIn (                    builder);
    case CellSymbol::OUT  : return mapOut(links,              builder);
    case CellSymbol::ZERO : return mapVal(       false,       builder);
    case CellSymbol::ONE  : return mapVal(       true,        builder);
    case CellSymbol::BUF  : return mapBuf(links,              builder);
    case CellSymbol::AND  : return mapAnd(links,      n0, n1, builder);
    case CellSymbol::OR   : return mapOr (links, inv, n0, n1, builder);
    case CellSymbol::XOR  : return mapXor(links,      n0, n1, builder);
    case CellSymbol::MAJ  : return mapMaj(links, inv, n0, n1, builder);
    default: assert(false && "Unknown gate");
  }
}

LinkList AigMapper::getNewLinks(const CellIdMap &oldToNew, size_t idx,
                                const Subnet &oldSubnet, const Entries &cells,
                                size_t &n0, size_t &n1,
                                InvCells &toInvert) const {

  LinkList links = oldSubnet.getLinks(idx);
  for (auto &link : links) {
    const size_t oldId = link.idx;

    const auto &cell = cells[oldId].cell;
    const auto symbol = cell.getSymbol();
    bool isZero = ((symbol == CellSymbol::ZERO) && !link.inv) ||
                  ((symbol == CellSymbol::ONE) && link.inv);
    bool isOne = ((symbol == CellSymbol::ONE) && !link.inv) ||
                 ((symbol == CellSymbol::ZERO) && link.inv);
    if (isZero) n0++;
    if (isOne) n1++;

    const auto search = oldToNew.find(oldId);
    assert(search != oldToNew.end() && "Old cell ID not found");

    const size_t cellId = search->second;
    bool needInvert = toInvert.find(cellId) != toInvert.end();
    needInvert = needInvert != link.inv;

    link.idx = cellId;
    link.inv = needInvert;
  }

  return links;
}

size_t AigMapper::mapIn(Builder &builder) const {
  return builder.addInput();
}

size_t AigMapper::mapOut(const LinkList &links, Builder &builder) const {
  assert(links.size() == 1 && "Only single input is allowed in OUT cell");
  return builder.addOutput(links.front());
}

size_t AigMapper::mapVal(bool val, Builder &builder) const {
    if (val) {
      return builder.addCell(CellSymbol::ONE);
    }
    return builder.addCell(CellSymbol::ZERO);
}

size_t AigMapper::mapBuf(const LinkList &links, Builder &builder) const {
    assert(links.size() == 1 && "Only single input is allowed in BUF cell");
    return builder.addCell(CellSymbol::BUF, links);
}

size_t AigMapper::mapAnd(const LinkList &links, size_t n0, size_t n1,
                         Builder &builder) const {

  if (links.size() == 1) {
    return mapBuf(links, builder);
  }
  if (n0 > 0) {
    return mapVal(false, builder);
  }
  if (n1 == links.size()) {
    return mapVal(true, builder);
  }
  return builder.addCellTree(CellSymbol::AND, links, 2);
}

size_t AigMapper::mapOr(LinkList &links, bool &inv, size_t n0, size_t n1,
                        Builder &builder) const {

  if (links.size() == 1) {
    return mapBuf(links, builder);
  }
  if (n1 > 0) {
    return mapVal(true, builder);
  }
  if (n0 == links.size()) {
    return mapVal(false, builder);
  }

  inv = true;
  // OR(x[1],...,x[n]) = ~(AND(~x[1],...,~x[n])).
  for (auto &link : links) {
    link.inv = !link.inv;
  }
  return mapAnd(links, 0, 0, builder);
}

size_t AigMapper::mapXor(LinkList &links, size_t n0, size_t n1,
                         Builder &builder) const {

  const size_t linksSize = links.size();
  if (linksSize == 1) {
    return mapBuf(links, builder);
  }
  if (n0 == linksSize) {
    return mapVal(false, builder);
  }
  if (n1 == linksSize) {
    return mapVal(n1 & 1u, builder);
  }

  links.reserve(2 * linksSize - 1);

  size_t l = 0;
  size_t r = 1;
  while (r < links.size()) {
    // XOR(x,y) = AND(~AND(x,y),~AND(~x,~y)).
    const Link x = links[l];
    const Link y = links[r];

    const Link nx(links[l].idx, !links[l].inv);
    const Link ny(links[r].idx, !links[r].inv);

    const size_t andID1 = builder.addCell(CellSymbol::AND, x, y);
    const size_t andID2 = builder.addCell(CellSymbol::AND, nx, ny);

    const Link l1(andID1, true);
    const Link l2(andID2, true);

    links.emplace_back(builder.addCell(CellSymbol::AND, l1, l2));

    l += 2;
    r += 2;
  }

  return links[l].idx;
}

size_t AigMapper::mapMaj(LinkList &links, bool &inv, size_t n0, size_t n1,
                         Builder &builder) const {

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
  /// TODO: Add a functionality for MAJ cells any arity.
  assert(linksSize == 3 && "Unsupported number of links in MAJ cell");

  return addMaj3(links, inv, builder);
}

size_t AigMapper::addMaj3(LinkList &links, bool &inv, Builder &builder) const {
  // MAJ(x,y,z)=OR(AND(x,y), AND(y,z), AND(z,x))
  const size_t and1 = builder.addCell(CellSymbol::AND, links[0], links[1]);
  const size_t and2 = builder.addCell(CellSymbol::AND, links[1], links[2]);
  const size_t and3 = builder.addCell(CellSymbol::AND, links[2], links[0]);

  const Link l1(and1);
  const Link l2(and2);
  const Link l3(and3);

  links[0] = l1;
  links[1] = l2;
  links[2] = l3;

  return mapOr(links, inv, 0, 0, builder);
}

} // namespace eda::gate::transformer
