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

struct TechMapCommand final : public UtopiaCommandBase<TechMapCommand> {
  using Indicator = eda::gate::criterion::Indicator;
  using Objective = eda::gate::criterion::Objective;

  TechMapCommand(): UtopiaCommandBase(
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

    UTOPIA_SHELL_ERROR_IF_NO_DESIGN(interp);

    if (!eda::gate::library::LibraryParser::get().isInit()) {
      return makeError(interp, "library has not been loaded");
    }

    if (getDesign()->isTechMapped()) {
      return makeError(interp, "design has been already techmapped");
    }

    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);

    for (size_t i = 0; i < getDesign()->getSubnetNum(); ++i) {
      const auto &subnetBuilder = getDesign()->getSubnetBuilder(i);
      const auto techmapBuilder = techMap(Objective(indicator), subnetBuilder);
      UTOPIA_SHELL_ERROR_IF(interp, !techmapBuilder, "returned null");
      getDesign()->setSubnetBuilder(i, techmapBuilder);
    }

    UTOPIA_SHELL_ERROR_IF(interp, !validateDesign(*getDesign(), logger),
        "validation checks failed");

    return TCL_OK;
  }

  Indicator indicator = Indicator::AREA;
};

} // namespace eda::shell
