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

struct DeleteDesignCommand final :
    public UtopiaCommandBase<DeleteDesignCommand> {
  DeleteDesignCommand(): UtopiaCommandBase(
      "delete_design", "Erases the design from memory") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_ERROR_IF_NO_DESIGN(interp);
    deleteDesign();
    return TCL_OK;
  }
};

} // namespace eda::shell
