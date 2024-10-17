//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/validator.h"
#include "gate/techmapper/design_unmapper.h"
#include "shell/shell.h"

namespace eda::shell {

struct UnmapCommand final : public UtopiaCommand {
  UnmapCommand(): UtopiaCommand(
      "unmap", "Performs technology unmapping") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    using namespace eda::gate::techmapper;

    if (!context->design) {
      context->design = getDesign();
    }

    UTOPIA_SHELL_ERROR_IF(interp, !context->design,
        "design has not been loaded");
    UTOPIA_SHELL_ERROR_IF(interp, !context->design->isTechMapped(),
        "design has not been techmapped");

    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);

    DesignUnmapper unmapper("unmap");
    unmapper.transform(context->design);
    
    UTOPIA_SHELL_ERROR_IF(interp, !validateDesign(*context->design, logger),
        "validation checks failed");

    return TCL_OK;
  }
};

} // namespace eda::shell
