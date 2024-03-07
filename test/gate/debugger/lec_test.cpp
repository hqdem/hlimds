//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "lec_test.h"
#include "util/logging.h"

using namespace eda::gate::debugger;
using namespace eda::gate::model;
using namespace eda::gate::parser;

namespace eda::gate::debugger {

CheckerResult fileLecTest(const std::string &fileName,
                          LecType lecType,
                          PreBasis basis,
                          const std::string &subPath) {

  GNet compiledNet = getModel(fileName, subPath);

  compiledNet.sortTopologically();
  std::shared_ptr<GNet> initialNet = std::make_shared<GNet>(compiledNet);

  GateIdMap gatesMap;
  std::shared_ptr<GNet> premappedNet = premap(initialNet, gatesMap, basis);
  return getChecker(lecType).equivalent(*initialNet, *premappedNet, gatesMap);
}

CheckerResult twoFilesLecTest(const std::string &fileName1,
                              const std::string &fileName2,
                              LecType lecType,
                              const std::string &subPath1,
                              const std::string &subPath2) {

  GNet compiledNet1 = getModel(fileName1, subPath1);
  GNet compiledNet2 = getModel(fileName2, subPath2);

  compiledNet1.sortTopologically();
  compiledNet2.sortTopologically();

  std::vector<GateId> inputs1;
  std::vector<GateId> inputs2;
  std::vector<GateId> outputs1;
  std::vector<GateId> outputs2;
  std::unordered_map<GateId, GateId> inOutMap;

  for (auto link : compiledNet1.sourceLinks()) {
    inputs1.push_back(link.target);
  }
  for (auto link : compiledNet2.sourceLinks()) {
    inputs2.push_back(link.target);
  }
  for (auto link : compiledNet1.targetLinks()) {
    outputs1.push_back(link.source);
  }
  for (auto link : compiledNet2.targetLinks()) {
    outputs2.push_back(link.source);
  }
  for (size_t i = 0; i < inputs1.size(); i++) {
    inOutMap[inputs1[i]] = inputs2[i];
  }
  for (size_t i = 0; i < outputs1.size(); i++) {
    inOutMap[outputs1[i]] = outputs2[i];
  }

  return getChecker(lecType).equivalent(compiledNet1, compiledNet2, inOutMap);
}

SatChecker::Hints checkerTestHints(unsigned N,
                                   const GNet &lhs,
                                   const Gate::SignalList &lhsInputs,
                                   Gate::Id lhsOutputId,
                                   const GNet &rhs,
                                   const Gate::SignalList &rhsInputs,
                                   Gate::Id rhsOutputId) {
  using Link = Gate::Link;
  using GateBinding = SatChecker::GateBinding;

  GateBinding imap, omap;

  LOG_DEBUG(lhs);
  LOG_DEBUG(rhs);

  // Input bindings.
  for (unsigned i = 0; i < N; i++) {
    imap.insert({Link(lhsInputs[i].node()), Link(rhsInputs[i].node())});
  }

  // Output bindings.
  omap.insert({Link(lhsOutputId), Link(rhsOutputId)});

  SatChecker::Hints hints;
  hints.sourceBinding = std::make_shared<GateBinding>(std::move(imap));
  hints.targetBinding = std::make_shared<GateBinding>(std::move(omap));

  return hints;
}

GateIdMap checkerTestMap(unsigned N,
                         const GNet &lhs,
                         const Gate::SignalList &lhsInputs,
                         Gate::Id lhsOutputId,
                         const GNet &rhs,
                         const Gate::SignalList &rhsInputs,
                         Gate::Id rhsOutputId) {
  GateIdMap gmap = {};

  // Input bindings.
  for (unsigned i = 0; i < N; i++) {
    gmap[lhsInputs[i].node()] = rhsInputs[i].node();
  }

  // Output bindings.
  gmap[lhsOutputId] = rhsOutputId;

  return gmap;
}

bool checkEquivTest(unsigned N,
                    const GNet &lhs,
                    const Gate::SignalList &lhsInputs,
                    Gate::Id lhsOutputId,
                    const GNet &rhs,
                    const Gate::SignalList &rhsInputs,
                    Gate::Id rhsOutputId) {
  SatChecker::Hints hints = checkerTestHints(N, lhs, lhsInputs, lhsOutputId,
                                             rhs, rhsInputs, rhsOutputId);
  return static_cast<SatChecker&>(
         getChecker(options::SAT)).equivalent(lhs, rhs, hints).equal();
}

bool checkEquivMiterTest(unsigned N,
                         GNet &lhs,
                         const Gate::SignalList &lhsInputs,
                         Gate::Id lhsOutputId,
                         GNet &rhs,
                         const Gate::SignalList &rhsInputs,
                         Gate::Id rhsOutputId) {
  GateIdMap gmap = checkerTestMap(N, lhs, lhsInputs, lhsOutputId,
                                  rhs, rhsInputs, rhsOutputId);
  GNet mit = *miter(lhs, rhs, gmap);
  return static_cast<SatChecker&>(
         getChecker(options::SAT)).isEqualCombMiter(mit).equal();
}

bool checkNorNorTest(unsigned N) {
  // ~(x1 | ... | xN).
  Gate::SignalList lhsInputs;
  Gate::Id lhsOutputId;
  auto lhs = makeNor(N, lhsInputs, lhsOutputId);

  // ~(x1 | ... | xN).
  Gate::SignalList rhsInputs;
  Gate::Id rhsOutputId;
  auto rhs = makeNor(N, rhsInputs, rhsOutputId);

  return checkEquivTest(N, *lhs, lhsInputs, lhsOutputId,
                           *rhs, rhsInputs, rhsOutputId) &&
         checkEquivMiterTest(N, *lhs, lhsInputs, lhsOutputId,
                                *rhs, rhsInputs, rhsOutputId);
}

bool checkNorAndnTest(unsigned N) {
  // ~(x1 | ... | xN).
  Gate::SignalList lhsInputs;
  Gate::Id lhsOutputId;
  auto lhs = makeNor(N, lhsInputs, lhsOutputId);

  // (~x1 & ... & ~xN).
  Gate::SignalList rhsInputs;
  Gate::Id rhsOutputId;
  auto rhs = makeAndn(N, rhsInputs, rhsOutputId);

  return checkEquivTest(N, *lhs, lhsInputs, lhsOutputId,
                           *rhs, rhsInputs, rhsOutputId) &&
         checkEquivMiterTest(N, *lhs, lhsInputs, lhsOutputId,
                                *rhs, rhsInputs, rhsOutputId);
}

bool checkNorAndTest(unsigned N) {
  // ~(x1 | ... | xN).
  Gate::SignalList lhsInputs;
  Gate::Id lhsOutputId;
  auto lhs = makeNor(N, lhsInputs, lhsOutputId);

  // (x1 & ... & xN).
  Gate::SignalList rhsInputs;
  Gate::Id rhsOutputId;
  auto rhs = makeAnd(N, rhsInputs, rhsOutputId);

  return checkEquivTest(N, *lhs, lhsInputs, lhsOutputId,
                           *rhs, rhsInputs, rhsOutputId) &&
         checkEquivMiterTest(N, *lhs, lhsInputs, lhsOutputId,
                                *rhs, rhsInputs, rhsOutputId);
}
} // namespace eda::gate::debugger
