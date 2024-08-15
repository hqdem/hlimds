//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/premapper/premapper.h"

namespace eda::gate::premapper {

using Link             = Premapper::Link;
using LinkList         = model::Subnet::LinkList;
using SubnetBuilder    = Premapper::SubnetBuilder;
using SubnetBuilderPtr = Premapper::SubnetBuilderPtr;

SubnetBuilderPtr Premapper::map(const SubnetBuilderPtr &builder) const {
  SubnetBuilder *builderPtr = builder.get();
  for (SafePasser iter = --builderPtr->end();
       !builderPtr->getCell(*iter).isIn(); --iter) {

    const size_t entryID = *iter;
    const auto &cell = builderPtr->getCell(entryID);
    assert(cell.arity <= Cell::InPlaceLinks && "Too great cell arity");

    bool skip = cell.isZero() || cell.isOne() || cell.isBuf() || cell.isOut();
    switch (basis) {
      case XAG: skip |= cell.isXor(); [[fallthrough]];
      case AIG: skip |= cell.isAnd(); break;
      case XMG: skip |= cell.isXor(); [[fallthrough]];
      case MIG: skip |= cell.isMaj(); break;
      default: assert(false && "Invalid basis for the premapper!");
    }
    if (skip) continue;

    const auto cutView = optimizer::getReconvergentCut(*builderPtr, entryID, k);
    const auto mffc = optimizer::getMffc(*builderPtr, cutView);
    const InOutMapping &iomapping = mffc.getInOutMapping();
    if (iomapping.inputs == iomapping.outputs) {
      if (cell.arity > k) {
        decomposeCell(builder, iter, entryID);
      } else { // Case when TFI of the entry are constants.
        constantCase(builder, iter, entryID);
      }
      continue;
    }

    SubnetObject rhs = resynthesizer.resynthesize(mffc, arity);
    assert(!rhs.isNull() && "Subnet wasn't synthesized!");

    iter.replace(rhs, iomapping);
  }
  return builder;
}

static Link addMaj5(SubnetBuilder &builder, const LinkList &links) {
  // <xyztu> = < <xyz> t <<xyu>uz> >
  LinkList tmp(links.begin(), links.begin() + 3);
  const Link xyzLink = builder.addCell(model::CellSymbol::MAJ, tmp);

  tmp[0] = links[0];
  tmp[1] = links[1];
  tmp[2] = links[4];
  const Link xyuLink = builder.addCell(model::CellSymbol::MAJ, tmp);
  // <<xyu>uz>
  tmp[0] = links[2];
  tmp[1] = xyuLink;
  tmp[2] = links[4];
  const Link muzLink = builder.addCell(model::CellSymbol::MAJ, tmp);

  tmp[0] = xyzLink;
  tmp[1] = links[3];
  tmp[2] = muzLink;
  return builder.addCell(model::CellSymbol::MAJ, tmp);
}

static Link decomposeMaj(SubnetBuilder &builder, const LinkList &links) {
  if (links.size() == 5) {
    return addMaj5(builder, links);
  }
  assert(false && "Unsupported number of links in MAJ cell");
}

void Premapper::decomposeCell(const SubnetBuilderPtr &builder,
                              SafePasser &iter,
                              const size_t entryID) const {

  InOutMapping iomapping;
  SubnetObject object;
  SubnetBuilder &rhs{object.builder()};
  SubnetBuilder *builderPtr = builder.get();

  const auto &cell = builderPtr->getCell(entryID);
  auto links = rhs.addInputs(cell.arity);
  for (size_t i = 0; i < links.size(); ++i) {
    const auto link = builderPtr->getLink(entryID, i);
    iomapping.inputs.push_back(link.idx);
    links[i].inv = link.inv;
  }
  iomapping.outputs.push_back(entryID);

  Link outLink;
  if (cell.isMaj()) {
    outLink = decomposeMaj(rhs, links);
  } else {
    outLink = rhs.addCellTree(cell.getSymbol(), links, 2);
  }
  rhs.addOutput(outLink);

  bool skipPremapping = false;
  switch (basis) {
    case XAG: skipPremapping |= cell.isXor(); [[fallthrough]];
    case AIG: skipPremapping |= cell.isAnd(); break;
    case XMG: skipPremapping |= cell.isXor(); [[fallthrough]];
    case MIG: skipPremapping |= cell.isMaj(); break;
    default: assert(false && "Invalid basis for the premapper!");
  }
  if (skipPremapping) {
    iter.replace(object, iomapping);
    return;
  }

  Premapper premapper("tmp", basis, resynthesizer, k);
  const auto rhsPtr = premapper.map(std::make_shared<SubnetBuilder>(rhs));
  iter.replace(*(rhsPtr.get()), iomapping);
}

void Premapper::constantCase(const SubnetBuilderPtr &builder,
                             SafePasser &iter,
                             const size_t entryID) const {

  SubnetBuilder *builderPtr = builder.get();

  InOutMapping iomapping(std::vector<size_t>{0}, std::vector<size_t>{entryID});
  model::SubnetView view(*builderPtr, iomapping);
  SubnetObject rhs = resynthesizer.resynthesize(view, arity);
  assert(!rhs.isNull() && "Subnet wasn't synthesized!");
  iter.replace(rhs, iomapping);
}

} // namespace eda::gate::premapper
