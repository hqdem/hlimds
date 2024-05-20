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
    const Subnet &subnet, SubnetBuilder &builder, IdxToLink &map) {
  const auto entries = subnet.getEntries();
  for (size_t i = subnet.getInNum(); i < subnet.size(); i++) {
    const auto &cell = entries[i].cell;

    LinkList newLinks;
    for (size_t j = 0; j < cell.arity; ++j) {
      Link oldLink = subnet.getLink(i, j);
      Link newLink{map[oldLink.idx].idx, oldLink.inv != 0};
      newLinks.push_back(newLink);
    }

    if (cell.isOut()) {
      map[i] = newLinks.front();
    } else {
      map[i] = builder.addCell(cell.getTypeID(), newLinks);
    }

    i += cell.more;
  }
}

static bool areMiterable(const Subnet &subnet1, const Subnet &subnet2) {
  const auto nIn1 = subnet1.getInNum();
  const auto nIn2 = subnet2.getInNum();

  if (nIn1 != nIn2) {
    CHECK(false) << "Different numbers of inputs: "
                 << nIn1 << " != " << nIn2 << std::endl;
    return false;
  }

  const auto nOut1 = subnet1.getOutNum();
  const auto nOut2 = subnet2.getOutNum();

  if (nOut1 != nOut2) {
    CHECK(false) << "Different numbers of outputs: "
                 << nOut1 << " != " << nOut2 << std::endl;
    return false;
  }

  return true;
}

void BaseChecker::makeMiter(SubnetBuilder &builder,
                            const SubnetID subnetID1,
                            const SubnetID subnetID2,
                            const CellToCell &mapping) {
  const Subnet &subnet1 = Subnet::get(subnetID1);
  const Subnet &subnet2 = Subnet::get(subnetID2);

  if (!areMiterable(subnet1, subnet2)) {
    return;
  }

  IdxToLink map1, map2;

  for (size_t i = 0; i < subnet1.getInNum(); ++i) {
    const auto idx1 = i;
    const auto idx2 = mapping.find(idx1)->second;
    map1[idx1] = map2[idx2] = builder.addInput();
  }

  buildCells(subnet1, builder, map1);
  buildCells(subnet2, builder, map2);

  LinkList xors(subnet1.getOutNum());
  for (size_t i = 0; i < subnet1.getOutNum(); ++i) {
    const auto idx1 = subnet1.size() + i - subnet1.getOutNum();
    const auto idx2 = mapping.find(idx1)->second;
    xors[i] = builder.addCell(CellSymbol::XOR, map1[idx1], map2[idx2]);
  }

  if (xors.size() == 1) {
    builder.addOutput(xors[0]);
  } else {
    builder.addOutput(builder.addCellTree(CellSymbol::OR, xors, 2));
  }
}

} // namespace eda::gate::debugger
