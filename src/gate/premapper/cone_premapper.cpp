//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/premapper/cone_premapper.h"

namespace eda::gate::premapper {

using Link              = ConePremapper::Link;
using LinkList          = model::Subnet::LinkList;
using ResynthesizerBase = ConePremapper::ResynthesizerBase;
using SubnetBuilder     = ConePremapper::SubnetBuilder;
using SubnetBuilderPtr  = ConePremapper::SubnetBuilderPtr;

int ConePremapper::resynthesize(const SubnetBuilder *builderPtr,
                                SafePasser &iter,
                                const SubnetView &view,
                                bool mayOptimize) const {

  auto rhs = resynthesizer.resynthesize(view, arity);
  int gain = builderPtr->evaluateReplace(rhs, view.getInOutMapping()).size;
  assert(!rhs.isNull() && "Subnet wasn't synthesized!");

  if ((gain > 0) || !mayOptimize) {
    iter.replace(rhs, view.getInOutMapping());
    return gain;
  }
  return 0;
}

SubnetBuilderPtr ConePremapper::map(const SubnetBuilderPtr &builder) const {
  SubnetBuilder *builderPtr = builder.get();
  if (builderPtr->begin() == builderPtr->end()) {
    return builder;
  }
  int64_t gain = 0;
  for (SafePasser iter = --builderPtr->end();
       !builderPtr->getCell(*iter).isIn() && (iter != builderPtr->begin());
       --iter) {

    const auto entryID = *iter;
    const auto &cell = builderPtr->getCell(entryID);
    assert(cell.arity <= Cell::InPlaceLinks && "Too great cell arity");

    bool skip = cell.isZero() || cell.isOne() || cell.isBuf() || cell.isOut();
    bool mayOptimize = !skip && (gain < 0);
    switch (basis) {
      case XAG: skip |= cell.isXor() && (cell.arity == 2); [[fallthrough]];
      case AIG: skip |= cell.isAnd() && (cell.arity == 2); break;
      case XMG: skip |= cell.isXor() && (cell.arity == 2); [[fallthrough]];
      case MIG: skip |= cell.isMaj() && (cell.arity == 3); break;
      default: assert(false && "Invalid basis for the premapper!");
    }
    if (skip && mayOptimize) {
      skip = false;
    } else {
      mayOptimize = false;
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
    if (mffc.getInNum() > k) {
      gain += resynthesize(builderPtr, iter, cutView, mayOptimize);
    } else {
      gain += resynthesize(builderPtr, iter, mffc, mayOptimize);
    }
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

void ConePremapper::decomposeCell(const SubnetBuilderPtr &builder,
                                  SafePasser &iter,
                                  const model::EntryID entryID) const {

  InOutMapping iomapping;
  const auto rhs = std::make_shared<SubnetBuilder>();
  SubnetBuilder *builderPtr = builder.get();
  SubnetBuilder *rhsPtr = rhs.get();

  const auto &cell = builderPtr->getCell(entryID);
  auto links = rhsPtr->addInputs(cell.arity);
  for (size_t i = 0; i < links.size(); ++i) {
    const auto link = builderPtr->getLink(entryID, i);
    iomapping.inputs.push_back(link.idx);
    links[i].inv = link.inv;
  }
  iomapping.outputs.push_back(entryID);

  Link outLink;
  if (cell.isMaj()) {
    outLink = decomposeMaj(*rhsPtr, links);
  } else {
    outLink = rhsPtr->addCellTree(cell.getSymbol(), links, 2);
  }
  rhsPtr->addOutput(outLink);

  bool skipPremapping = false;
  switch (basis) {
    case XAG: skipPremapping |= cell.isXor(); [[fallthrough]];
    case AIG: skipPremapping |= cell.isAnd(); break;
    case XMG: skipPremapping |= cell.isXor(); [[fallthrough]];
    case MIG: skipPremapping |= cell.isMaj(); break;
    default: assert(false && "Invalid basis for the premapper!");
  }
  if (skipPremapping) {
    iter.replace(*rhsPtr, iomapping);
    return;
  }

  ConePremapper premapper("tmp", basis, resynthesizer, k);
  premapper.map(rhs);
  iter.replace(*rhsPtr, iomapping);
}

void ConePremapper::constantCase(const SubnetBuilderPtr &builder,
                                 SafePasser &iter,
                                 const model::EntryID entryID) const {

  SubnetBuilder *builderPtr = builder.get();

  InOutMapping iomapping(model::EntryIDList{0}, model::EntryIDList{entryID});
  model::SubnetView view(*builderPtr, iomapping);
  SubnetObject rhs = resynthesizer.resynthesize(view, arity);
  assert(!rhs.isNull() && "Subnet wasn't synthesized!");
  iter.replace(rhs, iomapping);
}

} // namespace eda::gate::premapper
