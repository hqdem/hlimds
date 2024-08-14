//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "shell/shell.h"

namespace eda::shell {

struct GotoPointCommand final :
    public UtopiaCommandBase<GotoPointCommand> {
  GotoPointCommand(): UtopiaCommandBase(
      "goto_point", "Rolls back to a checkpoint") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);

    if (app.remaining().empty()) {
      return makeError(interp, "no point specified");
    }

    const auto point = app.remaining().at(0);
    getDesign()->rollback(point);

    return TCL_OK;
  }
};

} // namespace eda::shell
