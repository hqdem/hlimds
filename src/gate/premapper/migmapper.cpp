//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <gate/premapper/migmapper.h>

namespace eda::gate::premapper {

using Link          = MigMapper::Link;
using LinkList      = MigMapper::LinkList;
using SubnetBuilder = MigMapper::SubnetBuilder;

Link MigMapper::mapAnd(const LinkList &links, bool &inv,
                       SubnetBuilder &builder) const {

  return addMajTree(CellSymbol::AND, links, builder);
}

Link MigMapper::mapOr(const LinkList &links, bool &inv,
                      SubnetBuilder &builder) const {

  return addMajTree(CellSymbol::OR, links, builder);
}

Link MigMapper::mapXor(const LinkList &links, bool &inv,
                       SubnetBuilder &builder) const {

  return addMajTree(CellSymbol::XOR, links, builder);
}

Link MigMapper::addMajTree(CellSymbol symbol, const LinkList &links,
                           SubnetBuilder &builder) const {

  const size_t arity = symbol == CellSymbol::XOR ? 3 : 2;

  if (arity >= links.size()) {
    return addMaj(symbol, links, builder);
  }

  LinkList linkList(links);
  linkList.reserve(2 * links.size() - 1);

  for (size_t i = 0; i < linkList.size() - 1;) {
    const size_t nRest = linkList.size() - i;
    const size_t nArgs = (nRest > arity) ? arity : nRest;

    LinkList args(nArgs);
    for (size_t j = 0; j < nArgs; ++j) {
      args[j] = linkList[i++];
    }

    linkList.emplace_back(addMaj(symbol, args, builder));
  }

  return linkList.back();
}

Link MigMapper::addMaj(CellSymbol symbol, const LinkList &links,
                       SubnetBuilder &builder) const {

  // XOR processed separatly.
  if (symbol == CellSymbol::XOR) {
    return addXor(links, builder);
  }

  Link val;
  switch (symbol) {
    case CellSymbol::AND: val = builder.addCell(CellSymbol::ZERO); break;
    case CellSymbol::OR : val = builder.addCell(CellSymbol::ONE ); break;
    case CellSymbol::MAJ:
      assert(links.size() == 3 && "Incorrect links number for MAJ cell");
      return builder.addCell(symbol, links);
    default:
      assert(false && "Incorrect cell symbol");
  }
  // AND, OR cells mapping.
  assert(links.size() == 2 && "Incorrect links number for AND, OR cells");
  return builder.addCell(CellSymbol::MAJ, links[0], links[1], val);
}

Link MigMapper::addXor(const LinkList &links, SubnetBuilder &builder) const {
  assert((links.size() == 2 || links.size() == 3) && "Incorrect links number");
  if (links.size() == 3) {
    // XOR(x, y, z) = MAJ(~MAJ(x, y, x), MAJ(x, y, ~z), z).
    Link maj1 = ~builder.addCell(CellSymbol::MAJ, links[0], links[1], links[2]);
    Link maj2 = builder.addCell(CellSymbol::MAJ, links[0], links[1], ~links[2]);
    return builder.addCell(CellSymbol::MAJ, maj1, maj2, links[2]);
  }
  LinkList linkList(links);
  // XOR(x, y) = AND(OR(x, y), OR(~x, ~y))
  Link or1 = addMaj(CellSymbol::OR, linkList, builder);

  linkList[0] = ~linkList[0];
  linkList[1] = ~linkList[1];
  Link or2 = addMaj(CellSymbol::OR,  linkList, builder);

  linkList[0] = or1;
  linkList[1] = or2;
  return addMaj(CellSymbol::AND, linkList, builder);
}

Link MigMapper::mapMaj(const LinkList &links, bool &inv,
                       SubnetBuilder &builder) const {

  size_t linksSize = links.size();

  if (linksSize == 3) {
    return addMaj(CellSymbol::MAJ, links, builder);
  }
  if (linksSize == 5) {
    return addMaj5(links, builder);
  }

  assert(false && "Unsupported number of links in MAJ cell");
  return Link(0);
}

Link MigMapper::addMaj5(const LinkList &links, SubnetBuilder &builder) const {
  assert(links.size() == 5 && "Invalid number of links for MAJ5 element");
  // <xyztu> = < <xyz> t <<xyu>uz> >
  LinkList tmp(links.begin(), links.begin() + 3);
  const Link xyzLink = addMaj(CellSymbol::MAJ, tmp, builder);

  tmp[0] = links[0];
  tmp[1] = links[1];
  tmp[2] = links[4];
  const Link xyuLink = addMaj(CellSymbol::MAJ, tmp, builder);
  // <<xyu>uz>
  tmp[0] = links[2];
  tmp[1] = xyuLink;
  tmp[2] = links[4];
  const Link muzLink = addMaj(CellSymbol::MAJ, tmp, builder);

  tmp[0] = xyzLink;
  tmp[1] = links[3];
  tmp[2] = muzLink;
  return addMaj(CellSymbol::MAJ, tmp, builder);
}

} // namespace eda::gate::premapper
