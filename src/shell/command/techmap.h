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
    app.add_option("--area-constraint", areaConstraint,
                   "Max area in um^2 (overrides SDC)")
        ->expected(1);
    app.add_option("--delay-constraint", delayConstraint,
                   "Max delay in ns (overrides SDC)")
        ->expected(1);
    app.add_option("--power-constraint", powerConstraint,
                   "Max power in uW (overrides SDC)")
        ->expected(1);
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

    constexpr gate::criterion::Cost maxCostValue {
      std::numeric_limits<gate::criterion::Cost>::max()};

    //TODO: fix default values (right now equals to FLOAT_MAX)
    gate::criterion::Constraints constraints = {
      Constraint(gate::criterion::AREA, 
                 std::isnan(areaConstraint) ? maxCostValue : areaConstraint),
      Constraint(gate::criterion::DELAY,
                 std::isnan(delayConstraint) ? maxCostValue : delayConstraint),
      Constraint(gate::criterion::POWER,
                 std::isnan(powerConstraint) ? maxCostValue : powerConstraint)};

    //TODO: don't overwrite once criterion is set in other places
    if (!context->criterion) {
      context->criterion = std::make_unique<Criterion>(
          Objective(indicator), constraints);
    }

    techMapperWrapper tmw(*context, design);
    auto result = tmw.techMap();
    
    UTOPIA_SHELL_ERROR_IF(interp, !result.success,
      fmt::format("subnet '{}' returned nullptr", result.failedSubnet));

    UTOPIA_SHELL_ERROR_IF(interp, !validateDesign(design, logger),
        "validation checks failed");

    return TCL_OK;
  }

  Indicator indicator = Indicator::AREA;
  gate::criterion::Cost areaConstraint = NAN;
  gate::criterion::Cost delayConstraint = NAN;
  gate::criterion::Cost powerConstraint = NAN;
};

} // namespace eda::shell
