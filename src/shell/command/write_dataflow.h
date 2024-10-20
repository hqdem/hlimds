//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/printer/dataflow_printer.h"
#include "shell/shell.h"

#include <ostream>

namespace eda::shell {

struct WriteDataflowCommand final : public UtopiaCommand {
  WriteDataflowCommand(): UtopiaCommand(
      "write_dataflow", "Writes the dataflow graph to a DOT file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_SHELL_ERROR_IF_NO_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    if (auto status = createParentDirs(interp, fileName); status != TCL_OK) {
      return status;
    }

    std::ofstream out(fileName);
    out << *getDesign();

    return TCL_OK;
  }
};

} // namespace eda::shell
