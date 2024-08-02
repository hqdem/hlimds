//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "shell/shell.h"

struct ExitCommand final : public UtopiaCommandBase<ExitCommand> {
  ExitCommand(): UtopiaCommandBase<ExitCommand>(
      "exit", "Closes the interactive shell") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    return TCL_OK;
  }

  Tcl_CmdProc *getCmd() override {
    // Use the default processor.
    return nullptr;
  }
};
