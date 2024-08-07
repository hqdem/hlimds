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

struct SetNameCommand final : public UtopiaCommandBase<SetNameCommand> {
  SetNameCommand(): UtopiaCommandBase(
      "set_name", "Sets the design name") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);

    if (app.remaining().empty()) {
      return makeError(interp, "no name specified");
    }

    const auto name = app.remaining().at(0);
    designBuilder->setName(name);

    return TCL_OK;
  }
};

} // namespace eda::shell
