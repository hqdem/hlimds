//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "base_checker2.h"
#include "bdd_checker2.h"
#include "rnd_checker2.h"
#include "sat_checker2.h"

namespace eda::gate::debugger2 {

using IdxToLink = std::unordered_map<size_t, model::Subnet::Link>;
using LecType = eda::gate::debugger2::options::LecType;
using Link = model::Subnet::Link;
using LinkList = model::Subnet::LinkList;
using CellSymbol = model::CellSymbol;

BaseChecker2 &getChecker(LecType lec) {
  switch (lec) {
    case LecType::BDD: return BddChecker2::get();
    case LecType::RND: return RndChecker2::get();
    case LecType::SAT: return SatChecker2::get();
    default: return SatChecker2::get();
  }
}
BaseChecker2::~BaseChecker2() {};

void buildCells(const Subnet &net,
                SubnetBuilder &builder,
                IdxToLink &map) {
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

/// Gate-to-gate mapping between corresponding inputs and outputs of two nets.
struct MiterHints {
  /// Gate-to-gate mapping between inputs of two nets.
  CellToCell sourceBinding;
 /// Gate-to-gate mapping between outputs of two nets.
  CellToCell targetBinding;
};

bool areMiterable(const Subnet &net1,
                  const Subnet &net2,
                  const MiterHints &hints) {
  auto net1InNum = net1.getInNum();
  auto net2InNum = net2.getInNum();
  if (net1InNum != net2InNum) {
    CHECK(false) << "Different numbers of inputs: " <<
                    net1InNum << " != " << net2InNum << std::endl;
    return false;
  }

  auto net1OutNum = net1.getOutNum();
  auto net2OutNum = net2.getOutNum();
  if (net1OutNum != net2OutNum) {
    CHECK(false) << "Different numbers of outputs: " <<
                    net1OutNum << " != " << net2OutNum << std::endl;
    return false;
  }

  const CellToCell &sources = hints.sourceBinding;
  const CellToCell &targets = hints.targetBinding;
  if (sources.empty()) {
    CHECK(false) << "Zero inputs in hints" << std::endl;
    return false;
  }

  if (targets.empty()) {
    CHECK(false) << "Zero outputs in hints" << std::endl;
    return false;
  }

  if (net1InNum != sources.size()) {
    CHECK(false) << "Different numbers of inputs between nets and hints: " <<
                    net1InNum  << " != " << sources.size() << std::endl;
    return false;
  }

  if (net2OutNum != targets.size()) {
    CHECK(false) << "Different numbers of outputs between nets and hints: " <<
                    net2OutNum  << " != " << targets.size() << std::endl;
  }
  return true;
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

void BaseChecker2::miter2(SubnetBuilder &builder,
                          const SubnetID lhs,
                          const SubnetID rhs,
                          const CellToCell &gmap) {
  const size_t orArity = Subnet::Cell::InPlaceLinks;

  const Subnet &net1 = Subnet::get(lhs);
  const Subnet &net2 = Subnet::get(rhs);
  MiterHints hints = makeHints(net1, gmap);
  if (not areMiterable(net1, net2, hints)) {
    return;
  }

  IdxToLink map1, map2;

  auto itIn = hints.sourceBinding.begin();
  for (size_t i = 0; i < net1.getInNum(); ++i) {
    Link cellNum = builder.addInput();
    map1[itIn->first] = cellNum;
    map2[itIn->second] = cellNum;
    itIn++;
  }

  buildCells(net1, builder, map1);
  buildCells(net2, builder, map2);

  auto itOut = hints.targetBinding.begin();
  LinkList xors;
  for (size_t i = 0; i < net1.getOutNum(); i++) {
    LinkList xorLinks;
    xorLinks.push_back(map1[itOut->first]);
    xorLinks.push_back(map2[itOut->second]);
    xors.push_back(builder.addCell(CellSymbol::XOR, xorLinks));
    itOut++;
  }
  if (xors.size() == 1) {
    builder.addOutput(xors[0]);
  } else {
    Link orLink = builder.addCellTree(CellSymbol::OR, xors, orArity);
    builder.addOutput(orLink);
  }
}

} // namespace eda::gate::debugger2
