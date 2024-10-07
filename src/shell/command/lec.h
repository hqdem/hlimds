//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/base_checker.h"
#include "shell/shell.h"

namespace eda::shell {

struct LecCommand final : public UtopiaCommand {
  using BaseChecker = eda::gate::debugger::BaseChecker;
  using LecType = eda::gate::debugger::options::LecType;

  LecCommand(): UtopiaCommand(
      "lec", "Checks logical equivalence") {
    const std::map<std::string, LecType> lecMethodMap {
      { "bdd", LecType::BDD   },
      { "fra", LecType::FRAIG },
      { "rnd", LecType::RND   },
      { "sat", LecType::SAT   }
    };

    app.add_option("--method", method, "Method for checking equivalence")
        ->expected(1)
        ->transform(CLI::CheckedTransformer(lecMethodMap, CLI::ignore_case));
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);

    if (app.remaining().size() < 2) {
      return makeError(interp, "no points specified");
    }

    const auto point1 = app.remaining().at(0);
    const auto point2 = app.remaining().at(1);

    if (point1 == point2) {
      return makeError(interp, "equal points specified");
    }
    if (!getDesign()->hasPoint(point1)) {
      return makeError(interp, fmt::format("unknown point '{}'", point1));
    }
    if (!getDesign()->hasPoint(point2)) {
      return makeError(interp, fmt::format("unknown point '{}'", point2));
    }

    const auto &checker = BaseChecker::getChecker(method);
    const bool verdict = checker.areEquivalent(
        *getDesign(), point1, point2).equal();

    UTOPIA_SHELL_OUT << (verdict ? "Passed: " : "Failed: ")
                     << point1
                     << (verdict ? " == " : " != ")
                     << point2
                     << std::endl;

    if (!verdict) {
      return makeError(interp, "check failed");
    }

    return TCL_OK;
  }

  LecType method = LecType::SAT;
};

} // namespace eda::shell
