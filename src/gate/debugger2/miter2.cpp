//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
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
    if (curSymbol == CellSymbol::OUT) {
      map[i] = map[net.getLink(i, 0).idx];
    } else {
      map[i] = builder.addCell(curCell.getTypeID(), newLinks).idx;
    }
    i += curCell.more;
  }
}

bool areMiterable(const Subnet &net1,
                  const Subnet &net2,
                  const MiterHints &hints) {
  if (net1.getInNum() != net2.getInNum()) {
    CHECK(false) << "Nets do not have the same number of inputs" << std::endl;
    return false;
  }
  if (net1.getOutNum() != net2.getOutNum()) {
    CHECK(false) << "Nets do not have the same number of outputs" << std::endl;
    return false;
  }
  const CellToCell &sources = hints.sourceBinding;
  const CellToCell &targets = hints.targetBinding;
  if (net1.getInNum() != sources.size() || sources.empty()) {
    CHECK(false) << "Hints have incorrect number of inputs" << std::endl;
    return false;
  }
  if (net2.getOutNum() != targets.size() || targets.empty()) {
    CHECK(false) << "Hints have incorrect number of outputs" << std::endl;
    return false;
  }
  return true;
}

const Subnet &miter2(const Subnet &net1,
                     const Subnet &net2,
                     const CellToCell &gmap) {
  const size_t orArity = 2;

  MiterHints hints = makeHints(net1, gmap);
  if (not areMiterable(net1, net2, hints)) {
    return net1;
  }

  SubnetBuilder subnetBuilder;
  CellToCell map1, map2;

  auto itIn = hints.sourceBinding.begin();
  for (size_t i = 0; i < net1.getInNum(); ++i) {
    size_t cellNum = subnetBuilder.addInput().idx;
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
    xorLinks.push_back(Link(map1[itOut->first]));
    xorLinks.push_back(Link(map2[itOut->second]));
    xors.push_back(subnetBuilder.addCell(CellSymbol::XOR, xorLinks));
    itOut++;
  }
  if (xors.size() == 1) {
    subnetBuilder.addOutput(xors[0]);
  } else {
    Link orLink = subnetBuilder.addCellTree(CellSymbol::OR, xors, orArity);
    subnetBuilder.addOutput(orLink);
  }
  return Subnet::get(subnetBuilder.make());
}

MiterHints makeHints(const Subnet &net, const CellToCell &map) {
    MiterHints hints;
    size_t nOut = net.getOutNum();
    size_t nIn = net.getInNum();
    for (size_t i = 0; i < nIn; ++i) {
      hints.sourceBinding[i] = map.at(i);
    }
    for (size_t i = net.size() - 1; ; i--) {
      size_t prevMore = net.getEntries()[i - 1].cell.more;
      if (prevMore) {
        continue;
      }
      hints.targetBinding[i] = map.at(i);
      if (hints.targetBinding.size() == nOut) {
        break;
      }
    }
    return hints;
}

} // namespace eda::gate::debugger2
