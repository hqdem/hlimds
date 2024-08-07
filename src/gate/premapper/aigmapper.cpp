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

Link AigMapper::mapAnd(const LinkList &links, bool &inv,
                       SubnetBuilder &builder) const {

  return builder.addCellTree(CellSymbol::AND, links, 2);
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
