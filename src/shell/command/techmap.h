//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/validator.h"
#include "gate/techmapper/techmapper_wrapper.h"
#include "shell/shell.h"

namespace eda::shell {

struct TechMapCommand final : public UtopiaCommand {
  using Criterion = eda::gate::criterion::Criterion;
  using Constraint = eda::gate::criterion::Constraint;
  using Indicator = eda::gate::criterion::Indicator;
  using Objective = eda::gate::criterion::Objective;

  TechMapCommand(): UtopiaCommand(
      "techmap", "Performs technology mapping") {
    const std::map<std::string, Indicator> indicatorMap {
      { "area",  Indicator::AREA  },
      { "delay", Indicator::DELAY },
      { "power", Indicator::POWER },
    };

    app.add_option("--objective", indicator, "Optimization criterion")
        ->expected(1)
        ->transform(CLI::CheckedTransformer(indicatorMap, CLI::ignore_case));
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    using namespace eda::gate::techmapper;

    //TODO: remove with global variable,
    if (context->design == nullptr) {
      context->design = getDesign();
    }

    //TODO: this replaced UTOPIA_SHELL_ERROR_IF_NO_DESIGN
    //probably need another macro if used in other places
    UTOPIA_SHELL_ERROR_IF(interp, !context->design,
                          "design has not been loaded");
    UTOPIA_SHELL_ERROR_IF(interp, !context->techMapContext.library,
                          "library has not been loaded");

    auto& design = *(context->design);
    UTOPIA_SHELL_ERROR_IF(interp, design.isTechMapped(),
                          "design has been already techmapped");

    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);

    gate::criterion::Constraints constraints = {
        Constraint(gate::criterion::AREA,  45000),   // FIXME:
        Constraint(gate::criterion::DELAY, 1),       // FIXME:
        Constraint(gate::criterion::POWER, 25000)};  // FIXME:

    context->criterion = std::make_unique<Criterion>(
        Objective(indicator), constraints);
    
    techMapperWrapper tmw(*context, design);
    auto result = tmw.techMap();
    
    UTOPIA_SHELL_ERROR_IF(interp, !result.success,
      fmt::format("subnet '{}' returned nullptr", result.failedSubnet));

    UTOPIA_SHELL_ERROR_IF(interp, !validateDesign(design, logger),
        "validation checks failed");

    return TCL_OK;
  }

  Indicator indicator = Indicator::AREA;
};

} // namespace eda::shell
