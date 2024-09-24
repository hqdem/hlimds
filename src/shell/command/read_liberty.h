//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library.h"
#include "gate/library/library_factory.h"
#include "gate/library/readcells_srcfile_parser.h"
#include "shell/shell.h"

namespace eda::shell {

struct ReadLibertyCommand final : public UtopiaCommand {
  ReadLibertyCommand(): UtopiaCommand(
      "read_liberty", "Reads a library from a Liberty file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_SHELL_ERROR_IF_NO_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    UTOPIA_SHELL_ERROR_IF_FILE_NOT_EXIST(interp, fileName);

    eda::gate::library::ReadCellsParser parser(fileName);
    context->techMapContext.library =
      eda::gate::library::SCLibraryFactory::newLibraryUPtr(parser);
    
    return TCL_OK;
  }
};

} // namespace eda::shell
