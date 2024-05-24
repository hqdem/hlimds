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

#include <cassert>

namespace eda::gate::debugger {

using IdxToLink = std::unordered_map<size_t, model::Subnet::Link>;
using LecType = eda::gate::debugger::options::LecType;
using Link = model::Subnet::Link;
using LinkList = model::Subnet::LinkList;
using CellSymbol = model::CellSymbol;

BaseChecker &BaseChecker::getChecker(const LecType lec) {
  switch (lec) {
    case LecType::BDD:   return BddChecker::get();
    case LecType::FRAIG: return FraigChecker::get();
    case LecType::RND:   return RndChecker::get();
    case LecType::SAT:   return SatChecker::get();
    default: assert(false && "Unsupported LEC checker");
  }
  return SatChecker::get();
}

static void buildCells(const model::Subnet &subnet,
                       model::SubnetBuilder &builder,
                       IdxToLink &map) {
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

static void makeDefaultMapping(const model::Subnet &subnet1,
                               const model::Subnet &subnet2,
                               BaseChecker::CellToCell &mapping) {
  const auto nIn = subnet1.getInNum();
  const auto nOut = subnet1.getOutNum();

  assert(subnet2.getInNum() == nIn);
  assert(subnet2.getOutNum() == nOut);

  for (size_t i = 0; i < nIn; ++i) {
    mapping[i] = i;
  }

  const auto size1 = subnet1.size();
  const auto size2 = subnet2.size();
  for (size_t i = 0; i < nOut; ++i) {
    mapping[size1 + i - nOut] = size2 + i - nOut;
  }
}

void BaseChecker::makeMiter(model::SubnetBuilder &builder,
                            const model::Subnet &subnet1,
                            const model::Subnet &subnet2,
                            const CellToCell &mapping) {
  const auto nIn = subnet1.getInNum();
  const auto nOut = subnet1.getOutNum();

  assert(subnet2.getInNum() == nIn);
  assert(subnet2.getOutNum() == nOut);

  IdxToLink map1, map2;
  for (size_t i = 0; i < nIn; ++i) {
    const auto idx1 = i;
    const auto idx2 = mapping.find(idx1)->second;
    map1[idx1] = map2[idx2] = builder.addInput();
  }

  buildCells(subnet1, builder, map1);
  buildCells(subnet2, builder, map2);

  LinkList xors(subnet1.getOutNum());
  for (size_t i = 0; i < nOut; ++i) {
    const auto idx1 = subnet1.size() + i - nOut;
    const auto idx2 = mapping.find(idx1)->second;
    xors[i] = builder.addCell(CellSymbol::XOR, map1[idx1], map2[idx2]);
  }

  if (xors.size() == 1) {
    builder.addOutput(xors[0]);
  } else {
    builder.addOutput(builder.addCellTree(CellSymbol::OR, xors, 2));
  }
}

void BaseChecker::makeMiter(model::SubnetBuilder &builder,
                            const model::Subnet &subnet1,
                            const model::Subnet &subnet2) {
  CellToCell mapping;
  makeDefaultMapping(subnet1, subnet2, mapping);
  makeMiter(builder, subnet1, subnet2, mapping);
}

CheckerResult BaseChecker::areEquivalent(const model::Subnet &subnet1,
                                         const model::Subnet &subnet2,
                                         const CellToCell &mapping) const {
  model::SubnetBuilder builder;
  makeMiter(builder, subnet1, subnet2, mapping);
  return isSat(builder.make());
}

CheckerResult BaseChecker::areEquivalent(const model::Subnet &subnet1,
                                         const model::Subnet &subnet2) const {
  CellToCell mapping;
  makeDefaultMapping(subnet1, subnet2, mapping);
  return areEquivalent(subnet1, subnet2, mapping);
}

CheckerResult BaseChecker::areEquivalent(model::DesignBuilder &builder,
                                         const std::string &point1,
                                         const std::string &point2) const {
  for (size_t i = 0; i < builder.getSubnetNum(); ++i) {
    const auto subnetID1 = builder.getSubnetID(i, point1);
    const auto subnetID2 = builder.getSubnetID(i, point2);
    const auto result = areEquivalent(subnetID1, subnetID2);

    if (!result.equal()) {
      return result;
    }
  }

  return CheckerResult::EQUAL; 
}

} // namespace eda::gate::debugger
