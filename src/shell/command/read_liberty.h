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
      "read_liberty", "Reads a library from a Liberty files") {
    app.allow_extras();
  }

  using SCLibraryFactory = eda::gate::library::SCLibraryFactory;

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_SHELL_ERROR_IF_NO_FILES(interp, app);

    auto &libraryUptr = context->techMapContext.library;

    try {
      if (libraryUptr == nullptr) {
        libraryUptr = SCLibraryFactory::newLibraryUPtr();
      }
      for (const std::string &fileName : app.remaining()) {
        UTOPIA_SHELL_ERROR_IF_FILE_NOT_EXIST(interp, fileName);
        eda::gate::library::ReadCellsParser parser(fileName);
        //TODO: search map will be recalculated after each file is added
        if (!SCLibraryFactory::fillLibrary(*libraryUptr, parser)) {
          UTOPIA_SHELL_ERROR(interp, "Failed to fill library");
        }
      }
    } catch (std::exception &e) { //TODO: catch everything with (...)
      UTOPIA_SHELL_ERROR(interp, e.what());
    }
    return TCL_OK;
  }
};

} // namespace eda::shell
