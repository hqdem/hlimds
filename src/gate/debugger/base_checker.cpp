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

using CellSymbol = model::CellSymbol;
using IdxToLink = std::unordered_map<size_t, model::Subnet::Link>;
using LecType = eda::gate::debugger::options::LecType;
using Link = model::Subnet::Link;
using LinkList = model::Subnet::LinkList;
using SubnetView = model::SubnetView;

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

template<typename T>
void buildCell(const model::Subnet::Cell &cell,
               const uint16_t idx,
               const T &source,
               model::SubnetBuilder &builder,
               IdxToLink &map) {
  LinkList newLinks;
  for (size_t j = 0; j < cell.arity; ++j) {
    Link oldLink = source.getLink(idx, j);
    Link newLink{map[oldLink.idx].idx, oldLink.inv != 0};
    newLinks.push_back(newLink);
  }

  if (cell.isOut()) {
    map[idx] = newLinks.front();
  } else {
    map[idx] = builder.addCell(cell.getTypeID(), newLinks);
  }
}

static void buildCells(const model::Subnet &subnet,
                       model::SubnetBuilder &builder,
                       IdxToLink &map) {
  for (size_t i = subnet.getInNum(); i < subnet.size(); i++) {
    const auto &cell = subnet.getCell(i);
    buildCell(cell, i, subnet, builder, map);
    i += cell.more;
  }
}

static void buildCells(const model::SubnetBuilder &builder,
                       model::SubnetBuilder &miterBuilder,
                       IdxToLink &map) {
  for (auto it = builder.begin(); it != builder.end(); it.nextCell()) {
    const auto &cell = builder.getCell(*it);
    if (cell.isIn()) {
      continue;
    }
    buildCell(cell, *it, builder, miterBuilder, map);
  }
}

template<typename T>
uint16_t mapInputs(const T &source1,
                   const T &source2,
                   BaseChecker::CellToCell &mapping) {
  const auto nIn = source1.getInNum();
  const auto nOut = source1.getOutNum();

  assert(source2.getInNum() == nIn);
  assert(source2.getOutNum() == nOut);

  for (size_t i = 0; i < nIn; ++i) {
    mapping[i] = i;
  }
  return nOut;
}

static void makeDefaultMapping(const model::Subnet &subnet1,
                               const model::Subnet &subnet2,
                               BaseChecker::CellToCell &mapping) {
  const auto nOut = mapInputs(subnet1, subnet2, mapping);
  const auto size1 = subnet1.size();
  const auto size2 = subnet2.size();
  for (size_t i = 0; i < nOut; ++i) {
    mapping[size1 + i - nOut] = size2 + i - nOut;
  }
}

static void makeDefaultMapping(const model::SubnetBuilder &builder1,
                               const model::SubnetBuilder &builder2,
                               BaseChecker::CellToCell &mapping) {
  const auto nOut = mapInputs(builder1, builder2, mapping);
  const auto size1 = builder1.getCellNum();
  const auto size2 = builder2.getCellNum();
  for (size_t i = 0; i < nOut; ++i) {
    mapping[size1 + i - nOut] = size2 + i - nOut;
  }
}

template<typename T>
uint16_t makeNoOutMiter(model::SubnetBuilder &builder,
                        const T &source1,
                        const T &source2,
                        const BaseChecker::CellToCell &mapping,
                        IdxToLink &map1,
                        IdxToLink &map2) {
  const auto nIn = source1.getInNum();
  const auto nOut = source1.getOutNum();

  assert(source2.getInNum() == nIn);
  assert(source2.getOutNum() == nOut);

  for (size_t i = 0; i < nIn; ++i) {
    const auto idx1 = i;
    const auto idx2 = mapping.find(idx1)->second;
    map1[idx1] = map2[idx2] = builder.addInput();
  }

  buildCells(source1, builder, map1);
  buildCells(source2, builder, map2);
  return nOut;
}

void makeMiterOutputs(model::SubnetBuilder &builder, const LinkList &xors) {
  if (xors.size() == 1) {
    builder.addOutput(xors[0]);
  } else {
    builder.addOutput(builder.addCellTree(CellSymbol::OR, xors, 2));
  }
}

void BaseChecker::makeMiter(model::SubnetBuilder &builder,
                            const model::Subnet &subnet1,
                            const model::Subnet &subnet2,
                            const CellToCell &mapping) {
  IdxToLink map1, map2;
  const auto nOut = makeNoOutMiter(builder, subnet1, subnet2,
                                   mapping, map1, map2);
  LinkList xors(nOut);
  for (size_t i = 0; i < nOut; ++i) {
    const auto idx1 = subnet1.size() + i - nOut;
    const auto idx2 = mapping.find(idx1)->second;
    xors[i] = builder.addCell(CellSymbol::XOR, map1[idx1], map2[idx2]);
  }
  makeMiterOutputs(builder, xors);
}

void BaseChecker::makeMiter(model::SubnetBuilder &builder,
                            const model::Subnet &subnet1,
                            const model::Subnet &subnet2) {
  CellToCell mapping;
  makeDefaultMapping(subnet1, subnet2, mapping);
  makeMiter(builder, subnet1, subnet2, mapping);
}

void BaseChecker::makeMiter(model::SubnetBuilder &builder,
                            const model::SubnetBuilder &builder1,
                            const model::SubnetBuilder &builder2,
                            const CellToCell &mapping) {
  IdxToLink map1, map2;
  const auto nOut = makeNoOutMiter(builder, builder1, builder2,
                                   mapping, map1, map2);
  LinkList xors(nOut);
  for (size_t i = 0; i < nOut; ++i) {
    const auto idx1 = builder1.getCellNum() + i - nOut;
    const auto idx2 = mapping.find(idx1)->second;
    xors[i] = builder.addCell(CellSymbol::XOR, map1[idx1], map2[idx2]);
  }
  makeMiterOutputs(builder, xors);
}

void BaseChecker::makeMiter(model::SubnetBuilder &builder,
                            const model::SubnetBuilder &builder1,
                            const model::SubnetBuilder &builder2) {
  CellToCell mapping;
  makeDefaultMapping(builder1, builder2, mapping);
  makeMiter(builder, builder1, builder2, mapping);
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

CheckerResult BaseChecker::areEquivalent(const SubnetView &subnetView1,
                                         const SubnetView &subnetView2) const {
  //TODO: Implement view-based LEC method.
  return CheckerResult::UNKNOWN;
}

CheckerResult BaseChecker::areEquivalent(const model::SubnetBuilder &builder1,
                                         const model::SubnetBuilder &builder2,
                                         const CellToCell &mapping) const {
  model::SubnetBuilder builder;
  makeMiter(builder, builder1, builder2, mapping);
  return isSat(builder.make());
}

} // namespace eda::gate::debugger
