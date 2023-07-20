//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "lec_test.h"

using namespace eda::gate::debugger;
using namespace eda::gate::model;
using namespace eda::gate::parser::verilog;
using namespace eda::rtl::compiler;
using namespace eda::rtl::parser::ril;

namespace eda::gate::debugger {

GNet getNetRil(const std::string &fileName, const std::string &outSubPath) {
  std::filesystem::path basePath = std::getenv("UTOPIA_HOME");
  std::filesystem::path fullPath = basePath / outSubPath / fileName;

  auto model = parse(fullPath);
  Compiler compiler(FLibraryDefault::get());
  return *compiler.compile(*model);
}

GNet getModel(const std::string &fileName,
              const std::string &outSubPath,
              Exts ext) {
  switch (ext) {
  case Exts::VERILOG:
    return *parseVerilog(fileName);
  case Exts::RIL:
    return getNetRil(fileName, outSubPath);
  default:
    CHECK(false) << "Unsupported extension!\n";
    return GNet(0);
  }
}

Exts getExt(std::uint64_t pos, const std::string &fileName) {
  std::string extension = fileName.substr(pos);
  if (extension == ".v") {
    return Exts::VERILOG;
  } else if (extension == ".ril") {
    return Exts::RIL;
  }
  return Exts::UNSUP;
}

CheckerResult fileLecTest(const std::string &fileName,
                          BaseChecker &checker,
                          PreBasis basis,
                          const std::string &outSubPath) {
  std::uint64_t pos = fileName.rfind(".");
  if (pos == std::string::npos) {
    CHECK(false) << "Filename does not containt extension!\n";
    return CheckerResult::ERROR;
  }

  Exts ext = getExt(pos, fileName);
  if (ext == Exts::UNSUP) {
    CHECK(false) << "Unsupported HDL!\n";
    return CheckerResult::ERROR;
  }

  GNet compiledNet = getModel(fileName, outSubPath, ext);

  compiledNet.sortTopologically();
  std::shared_ptr<GNet> initialNet = std::make_shared<GNet>(compiledNet);

  GateIdMap gatesMap;
  std::shared_ptr<GNet> premappedNet = premap(initialNet, gatesMap, basis);
  return checker.equivalent(*initialNet, *premappedNet, gatesMap);
}

Checker::Hints checkEquivHints(unsigned N,
                               const GNet &lhs,
                               const Gate::SignalList &lhsInputs,
                               Gate::Id lhsOutputId,
                               const GNet &rhs,
                               const Gate::SignalList &rhsInputs,
                               Gate::Id rhsOutputId) {
  using Link = Gate::Link;
  using GateBinding = Checker::GateBinding;

  GateBinding imap, omap;

  std::cout << lhs << std::endl;
  std::cout << rhs << std::endl;

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
  Checker::Hints hints = checkEquivHints(N, lhs, lhsInputs, lhsOutputId,
                                            rhs, rhsInputs, rhsOutputId);
  return checker.equivalent(lhs, rhs, hints) == CheckerResult::EQUAL;
}

bool checkEquivMiterTest(unsigned N,
                         GNet &lhs,
                         const Gate::SignalList &lhsInputs,
                         Gate::Id lhsOutputId,
                         GNet &rhs,
                         const Gate::SignalList &rhsInputs,
                         Gate::Id rhsOutputId) {
  Checker checker;
  Checker::Hints hints = checkEquivHints(N, lhs, lhsInputs, lhsOutputId,
                                            rhs, rhsInputs, rhsOutputId);
  GNet mit = *miter(lhs, rhs, hints);
  return checker.isEqualCombMiter(mit) == CheckerResult::EQUAL;
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
