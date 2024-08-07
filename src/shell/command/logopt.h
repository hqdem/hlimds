//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/pass.h"
#include "shell/shell.h"

#define ADD_CUSTOM_CMD(app, name, desc, callback)\
  app.add_subcommand(name, desc)->parse_complete_callback([&]() {\
    measureAndRun(name, callback);\
  })

#define ADD_CMD(app, cmd, name, desc)\
  ADD_CUSTOM_CMD(app, name, desc, [&]() {\
    foreach(cmd())->transform(designBuilder);\
  })

namespace eda::shell {

template<typename Func>
static void measureAndRun(const std::string &name, Func func) {
  using clock = std::chrono::high_resolution_clock;

  const auto oldCellNum = std::get<2>(designBuilder->getCellNum());
  const auto start = clock::now();
  func();
  const auto end = clock::now();
  const auto newCellNum = std::get<2>(designBuilder->getCellNum());

  const auto delta = static_cast<int>(newCellNum) -
                     static_cast<int>(oldCellNum);

  const auto *sign = delta > 0 ? "+" : "";

  const auto percent = oldCellNum != 0 ?
      std::abs(100.f * delta / oldCellNum) : 0.f;

  printTime<clock>(name, start, end,
      /* prefix */ "  - ",
      /* suffix */ fmt::format(" -> {}{} [{:.2f}%]", sign, delta, percent));
}

struct LogOptCommand final : public UtopiaCommandBase<LogOptCommand> {
  LogOptCommand() : UtopiaCommandBase(
      "logopt", "Applies an optimization pass to the design") {
    namespace pass = eda::gate::optimizer;

    // Premapping.
    ADD_CMD(app, pass::aig, "aig", "Mapping to AIG");
    ADD_CMD(app, pass::mig, "mig", "Mapping to MIG");

    // Balancing.
    ADD_CMD(app, pass::b, "b", "Depth-aware balancing");

    // Rewriting.
    auto *passRw = ADD_CUSTOM_CMD(app,
        "rw", "Rewriting", [&]() {
      foreach(pass::rw(rwName, rwK, rwZ))->transform(designBuilder);
    });
    passRw->add_option("--name", rwName);
    passRw->add_option("-k", rwK);
    passRw->add_flag("-z", rwZ);

    ADD_CMD(app, pass::rwz, "rwz", "Rewriting w/ zero-cost replacements");

    // Refactoring.
    ADD_CMD(app, pass::rf,  "rf",  "Refactoring");
    ADD_CMD(app, pass::rfz, "rfz", "Refactoring w/ zero-cost replacements");
    ADD_CMD(app, pass::rfa, "rfa", "Area-aware refactoring");
    ADD_CMD(app, pass::rfd, "rfd", "Depth-aware refactoring");
    ADD_CMD(app, pass::rfp, "rfp", "Power-aware refactoring");

    // Resubstitution.
    auto *passRs = ADD_CUSTOM_CMD(app,
        "rs", "Resubstitution", [&]() {
      foreach(pass::rs(rsName, rsK, rsN))->transform(designBuilder);
    });
    passRs->add_option("--name", rsName);
    passRs->add_option("-k", rsK);
    passRs->add_option("-n", rsN);

    auto *passRsz = ADD_CUSTOM_CMD(app,
        "rsz", "Resubstitution w/ zero-cost replacements", [&]() {
      foreach(pass::rsz(rszName, rszK, rszN))->transform(designBuilder);
    });
    passRsz->add_option("--name", rszName);
    passRsz->add_option("-k", rszK);
    passRsz->add_option("-n", rszN);

    // Predefined scripts.
    ADD_CMD(app, pass::resyn,     "resyn",     "Predefined script resyn");
    ADD_CMD(app, pass::resyn2,    "resyn2",    "Predefined script resyn2");
    ADD_CMD(app, pass::resyn2a,   "resyn2a",   "Predefined script resyn2a");
    ADD_CMD(app, pass::resyn3,    "resyn3",    "Predefined script resyn3");
    ADD_CMD(app, pass::compress,  "compress",  "Predefined script compress");
    ADD_CMD(app, pass::compress2, "compress2", "Predefined script compress2");

    app.require_subcommand();
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    // Passes are executed as callbacks when parsing the arguments.
    return TCL_OK;
  }

  // Rewriter.
  std::string rwName = "rw";
  uint16_t rwK = 4;
  bool rwZ = false;

  // Resubstitutor.
  std::string rsName = "rs";
  uint16_t rsK = 8;
  uint16_t rsN = 16;

  // Resubstitutor w/ zero-cost replacements.
  std::string rszName = "rsz";
  uint16_t rszK = 8;
  uint16_t rszN = 16;
};

} // namespace eda::shell
