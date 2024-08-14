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

struct HelpCommand final : public UtopiaCommandBase<HelpCommand> {
  HelpCommand(): UtopiaCommandBase(
      "help", "Prints help information") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);

    if (app.remaining().empty()) {
      shell->printHelp(UTOPIA_SHELL_OUT);
      return TCL_OK;
    }

    auto name = app.remaining().at(0);
    auto *command = shell->getCommand(name);

    if (command) {
      command->printHelp(UTOPIA_SHELL_OUT);
      return TCL_OK;
    }

    return makeError(interp, fmt::format("unknown command '{}'", name));
  }
};

} // namespace eda::shell
