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

struct WriteDataflowCommand final :
    public UtopiaCommandBase<WriteDataflowCommand> {
  WriteDataflowCommand(): UtopiaCommandBase(
      "write_dataflow", "Writes the dataflow graph to a DOT file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_SHELL_ERROR_IF_NO_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    std::filesystem::path filePath = fileName;

    UTOPIA_SHELL_ERROR_IF(interp, !filePath.has_filename(),
        "path does not contain a file name");

    const std::string dir = filePath.remove_filename();
    if (!dir.empty()) {
      UTOPIA_SHELL_ERROR_IF(interp, !createDirectories(dir),
          fmt::format("cannot create directory '{}'", dir));
    }

    std::ofstream out(fileName);
    out << *getDesign();
    out.close();

    return TCL_OK;
  }
};

} // namespace eda::shell
