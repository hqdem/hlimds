//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger2/miter2.h"

namespace eda::gate::debugger2 {

void buildCells(const Subnet &net,
                SubnetBuilder &builder,
                CellToCell &map) {
  for (size_t i = net.getInNum(); i < net.size(); i++) {
    auto curCell = net.getEntries()[i].cell;
    auto curSymbol = curCell.getSymbol();
    LinkList newLinks;
    for (size_t j = 0; j < curCell.arity; ++j) {
      Link curLink = net.getLink(i, j);
      newLinks.push_back(Subnet::Link(map[curLink.idx], curLink.inv));
    }
    map[i] = builder.addCell(curSymbol == OUT ? BUF : curSymbol, newLinks);
    i += curCell.more;
  }
}

bool areMiterable(const Subnet &net1, const Subnet &net2, MiterHints &hints) {
  if (net1.getInNum() != net2.getInNum()) {
    CHECK(false) << "Nets do not have the same number of inputs" << std::endl;
    return false;
  }
  if (net1.getOutNum() != net2.getOutNum()) {
    CHECK(false) << "Nets do not have the same number of outputs" << std::endl;
    return false;
  }
  if (net1.getInNum() != hints.sourceBinding.size()) {
    CHECK(false) << "Hints have incorrect number of inputs" << std::endl;
    return false;
  }
  if (net2.getOutNum() != hints.targetBinding.size()) {
    CHECK(false) << "Hints have incorrect number of outputs" << std::endl;
    return false;
  }
  return true;
}

Subnet miter2(const Subnet &net1, const Subnet &net2, MiterHints &hints) {
  if (not areMiterable(net1, net2, hints)) {
    return net1;
  }

  SubnetBuilder subnetBuilder;
  CellToCell map1, map2;

  auto itIn = hints.sourceBinding.begin();
  for (size_t i = 0; i < net1.getInNum(); ++i) {
    size_t cellNum = subnetBuilder.addCell(IN, SubnetBuilder::INPUT);
    map1[itIn->first] = cellNum;
    map2[itIn->second] = cellNum;
    itIn++;
  }

  buildCells(net1, subnetBuilder, map1);
  buildCells(net2, subnetBuilder, map2);

  auto itOut = hints.targetBinding.begin();
  LinkList xors;
  for (size_t i = 0; i < net1.getOutNum(); i++) {
    LinkList xorLinks;
    xorLinks.push_back(map1[itOut->first]);
    xorLinks.push_back(map2[itOut->second]);
    xors.push_back(subnetBuilder.addCell(XOR, xorLinks));
    itOut++;
  }
  Link orLink = subnetBuilder.addCell(OR, xors);
  subnetBuilder.addCell(OUT, orLink, SubnetBuilder::OUTPUT);
  return Subnet::get(subnetBuilder.make());
}

} // namespace eda::gate::debugger2
