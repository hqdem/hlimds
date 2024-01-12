//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "lec_test.h"
#include "util/logging.h"

using namespace eda::gate::debugger;
using namespace eda::gate::model;
using namespace eda::gate::parser;

namespace eda::gate::debugger {

CheckerResult fileLecTest(const std::string &fileName,
                          BaseChecker &checker,
                          PreBasis basis,
                          const std::string &subPath) {
  Exts ext = getExt(fileName);
  if (ext == Exts::UNSUPPORTED) {
    CHECK(false) << "Unsupported HDL!" << std::endl;
    return CheckerResult::ERROR;
  }

  GNet compiledNet = getModel(fileName, subPath, ext);

  compiledNet.sortTopologically();
  std::shared_ptr<GNet> initialNet = std::make_shared<GNet>(compiledNet);

  GateIdMap gatesMap;
  std::shared_ptr<GNet> premappedNet = premap(initialNet, gatesMap, basis);
  return checker.equivalent(*initialNet, *premappedNet, gatesMap);
}

CheckerResult twoFilesLecTest(const std::string &fileName1,
                              const std::string &fileName2,
                              BaseChecker &checker,
                              const std::string &subPath1,
                              const std::string &subPath2) {
  Exts ext1 = getExt(fileName1);
  Exts ext2 = getExt(fileName2);
  if (ext1 == Exts::UNSUPPORTED || ext2 == Exts::UNSUPPORTED) {
    CHECK(false) << "Unsupported HDL!\n";
    return CheckerResult::ERROR;
  }

  GNet compiledNet1 = getModel(fileName1, subPath1, ext1);
  GNet compiledNet2 = getModel(fileName2, subPath2, ext2);

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

  return checker.equivalent(compiledNet1, compiledNet2, inOutMap);
}

Checker::Hints checkerTestHints(unsigned N,
                                const GNet &lhs,
                                const Gate::SignalList &lhsInputs,
                                Gate::Id lhsOutputId,
                                const GNet &rhs,
                                const Gate::SignalList &rhsInputs,
                                Gate::Id rhsOutputId) {
  using Link = Gate::Link;
  using GateBinding = Checker::GateBinding;

  GateBinding imap, omap;

  LOG_DEBUG(lhs);
  LOG_DEBUG(rhs);

  // Input bindings.
  for (unsigned i = 0; i < N; i++) {
    imap.insert({Link(lhsInputs[i].node()), Link(rhsInputs[i].node())});
  }

  // Output bindings.
  omap.insert({Link(lhsOutputId), Link(rhsOutputId)});

  Checker::Hints hints;
  hints.sourceBinding = std::make_shared<GateBinding>(std::move(imap));
  hints.targetBinding = std::make_shared<GateBinding>(std::move(omap));

  return hints;
}

bool checkEquivTest(unsigned N,
                    const GNet &lhs,
                    const Gate::SignalList &lhsInputs,
                    Gate::Id lhsOutputId,
                    const GNet &rhs,
                    const Gate::SignalList &rhsInputs,
                    Gate::Id rhsOutputId) {
  Checker checker;
  Checker::Hints hints = checkerTestHints(N, lhs, lhsInputs, lhsOutputId,
                                            rhs, rhsInputs, rhsOutputId);
  return checker.equivalent(lhs, rhs, hints).equal();
}

bool checkEquivMiterTest(unsigned N,
                         GNet &lhs,
                         const Gate::SignalList &lhsInputs,
                         Gate::Id lhsOutputId,
                         GNet &rhs,
                         const Gate::SignalList &rhsInputs,
                         Gate::Id rhsOutputId) {
  Checker checker;
  Checker::Hints hints = checkerTestHints(N, lhs, lhsInputs, lhsOutputId,
                                            rhs, rhsInputs, rhsOutputId);
  GNet mit = *miter(lhs, rhs, hints);
  return checker.isEqualCombMiter(mit).equal();
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
