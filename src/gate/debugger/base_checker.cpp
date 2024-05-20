//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/base_checker.h"
#include "gate/debugger/bdd_checker.h"
#include "gate/debugger/fraig_checker.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/debugger/sat_checker.h"

namespace eda::gate::debugger {

using IdxToLink = std::unordered_map<size_t, model::Subnet::Link>;
using LecType = eda::gate::debugger::options::LecType;
using Link = model::Subnet::Link;
using LinkList = model::Subnet::LinkList;
using CellSymbol = model::CellSymbol;

BaseChecker &getChecker(LecType lec) {
  switch (lec) {
    case LecType::BDD: return BddChecker::get();
    case LecType::FRAIG: return FraigChecker::get();
    case LecType::RND: return RndChecker::get();
    case LecType::SAT: return SatChecker::get();
    default: return SatChecker::get();
  }
}

static void buildCells(
    const Subnet &net, SubnetBuilder &builder, IdxToLink &map) {
  for (size_t i = net.getInNum(); i < net.size(); i++) {
    auto curCell = net.getEntries()[i].cell;
    auto curSymbol = curCell.getSymbol();
    LinkList newLinks;
    for (size_t j = 0; j < curCell.arity; ++j) {
      Link curLink = net.getLink(i, j);
      newLinks.push_back(Subnet::Link(map[curLink.idx].idx, curLink.inv));
    }
    if (curSymbol == CellSymbol::OUT) {
      map[i] = newLinks.front();
    } else {
      map[i] = builder.addCell(curCell.getTypeID(), newLinks);
    }
    i += curCell.more;
  }
}

bool areMiterable(const Subnet &net1, const Subnet &net2) {
  auto net1InNum = net1.getInNum();
  auto net2InNum = net2.getInNum();

  if (net1InNum != net2InNum) {
    CHECK(false) << "Different numbers of inputs: "
                 << net1InNum << " != " << net2InNum << std::endl;
    return false;
  }

  auto net1OutNum = net1.getOutNum();
  auto net2OutNum = net2.getOutNum();
  if (net1OutNum != net2OutNum) {
    CHECK(false) << "Different numbers of outputs: "
                 << net1OutNum << " != " << net2OutNum << std::endl;
    return false;
  }

  return true;
}

void BaseChecker::makeMiter(SubnetBuilder &builder,
                            const SubnetID lhs,
                            const SubnetID rhs,
                            const CellToCell &gmap) {
  const Subnet &net1 = Subnet::get(lhs);
  const Subnet &net2 = Subnet::get(rhs);

  if (not areMiterable(net1, net2)) {
    return;
  }

  IdxToLink map1, map2;

  for (size_t i = 0; i < net1.getInNum(); ++i) {
    const auto idx1 = i;
    const auto idx2 = gmap.find(idx1)->second;
    map1[idx1] = map2[idx2] = builder.addInput();
  }

  buildCells(net1, builder, map1);
  buildCells(net2, builder, map2);

  LinkList xors(net1.getOutNum());
  for (size_t i = 0; i < net1.getOutNum(); ++i) {
    const auto idx1 = net1.size() + i - net1.getOutNum();
    const auto idx2 = gmap.find(idx1)->second;
    xors[i] = builder.addCell(CellSymbol::XOR, map1[idx1], map2[idx2]);
  }

  if (xors.size() == 1) {
    builder.addOutput(xors[0]);
  } else {
    builder.addOutput(builder.addCellTree(CellSymbol::OR, xors, 2));
  }
}

} // namespace eda::gate::debugger
