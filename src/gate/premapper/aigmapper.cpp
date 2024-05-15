//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <gate/premapper/aigmapper.h>

namespace eda::gate::premapper {

using Link          = AigMapper::Link;
using LinkList      = AigMapper::LinkList;
using SubnetBuilder = AigMapper::SubnetBuilder;

std::unique_ptr<SubnetBuilder> AigMapper::make(const SubnetID subnetID) const {
  auto builder = std::make_unique<SubnetBuilder>();

  CellIdMap oldToNew;
  const auto &oldSubnet = Subnet::get(subnetID);
  const auto &entries = oldSubnet.getEntries();

  for (uint32_t i = 0; i < oldSubnet.size(); ++i) {
    const auto &cell = entries[i].cell;
    const auto symbol = cell.getSymbol();

    size_t n0 = 0;
    size_t n1 = 0;

    LinkList links = getNewLinks(oldToNew, i, oldSubnet, entries, n0, n1);

    bool inv = false;
    Link link = mapCell(symbol, links, inv, n0, n1, *builder);
    link.inv = link.inv != inv;
    oldToNew[i] = link;

    i += cell.more;
  }

  return builder;
}

Link AigMapper::mapCell(CellSymbol symbol, LinkList &links, bool &inv,
                        size_t n0, size_t n1, SubnetBuilder &builder) const {

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

LinkList AigMapper::getNewLinks(const CellIdMap &oldToNew, uint32_t idx,
                                const Subnet &oldSubnet, const Entries &entries,
                                size_t &n0, size_t &n1) const {

  LinkList links = oldSubnet.getLinks(idx);
  for (auto &link : links) {
    const uint32_t oldId = link.idx;

    const auto &cell = entries[oldId].cell;
    const auto symbol = cell.getSymbol();

    bool isZero = ((symbol == CellSymbol::ZERO) && !link.inv) ||
                  ((symbol == CellSymbol::ONE) && link.inv);
    bool isOne  = ((symbol == CellSymbol::ONE) && !link.inv) ||
                  ((symbol == CellSymbol::ZERO) && link.inv);

    if (isZero) n0++;
    if (isOne ) n1++;

    const auto search = oldToNew.find(oldId);
    assert(search != oldToNew.end() && "Old cell ID not found");

    const Link cellLink = search->second;
    link.idx = cellLink.idx;
    link.inv = cellLink.inv != link.inv;
  }

  return links;
}

Link AigMapper::mapIn(SubnetBuilder &builder) const {
  return builder.addInput();
}

Link AigMapper::mapOut(const LinkList &links, SubnetBuilder &builder) const {
  assert(links.size() == 1 && "Only single input is allowed in OUT cell");
  Link link = links.front();

  if (link.inv) {
    link = builder.addCell(CellSymbol::BUF, links);
  }

  return builder.addOutput(link);
}

Link AigMapper::mapVal(bool val, SubnetBuilder &builder) const {
  if (val) {
    return builder.addCell(CellSymbol::ONE);
  }
  return builder.addCell(CellSymbol::ZERO);
}

Link AigMapper::mapBuf(const LinkList &links, SubnetBuilder &builder) const {
  assert(links.size() == 1 && "Only single input is allowed in BUF cell");
  return builder.addCell(CellSymbol::BUF, links);
}

Link AigMapper::mapAnd(const LinkList &links, size_t n0, size_t n1,
                       SubnetBuilder &builder) const {

  const size_t linksSize = links.size();

  if (linksSize == 1) {
    return mapBuf(links, builder);
  }
  if (n0 > 0) {
    return mapVal(false, builder);
  }
  if (n1 == linksSize) {
    return mapVal(true, builder);
  }

  return builder.addCellTree(CellSymbol::AND, links, 2);
}

Link AigMapper::mapOr(LinkList &links, bool &inv, size_t n0, size_t n1,
                      SubnetBuilder &builder) const {

  const size_t linksSize = links.size();

  if (linksSize == 1) {
    return mapBuf(links, builder);
  }
  if (n1 > 0) {
    return mapVal(true, builder);
  }
  if (n0 == linksSize) {
    return mapVal(false, builder);
  }

  inv = true;
  // OR(x[1],...,x[n]) = ~(AND(~x[1],...,~x[n])).
  for (auto &link : links) {
    link.inv = !link.inv;
  }
  return mapAnd(links, 0, 0, builder);
}

Link AigMapper::mapXor(LinkList &links, size_t n0, size_t n1,
                       SubnetBuilder &builder) const {

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

    Link l1 = builder.addCell(CellSymbol::AND, x, y);
    Link l2 = builder.addCell(CellSymbol::AND, nx, ny);

    l1.inv = true;
    l2.inv = true;

    links.emplace_back(builder.addCell(CellSymbol::AND, l1, l2));

    l += 2;
    r += 2;
  }

  return links[l];
}

Link AigMapper::mapMaj(LinkList &links, bool &inv, size_t n0, size_t n1,
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
  /// TODO: Add a functionality for MAJ cells any arity.
  assert(linksSize == 3 && "Unsupported number of links in MAJ cell");

  return addMaj3(links, inv, builder);
}

Link AigMapper::addMaj3(LinkList &links, bool &inv,
                        SubnetBuilder &builder) const {

  Link link = links[0];
  // MAJ(x,y,z)=OR(AND(x,y), AND(y,z), AND(z,x))
  links[0] = builder.addCell(CellSymbol::AND, links[0], links[1]);
  links[1] = builder.addCell(CellSymbol::AND, links[1], links[2]);
  links[2] = builder.addCell(CellSymbol::AND, links[2], link    );

  return mapOr(links, inv, 0, 0, builder);
}

} // namespace eda::gate::premapper
