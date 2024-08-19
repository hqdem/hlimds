//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/validator.h"
#include "gate/translator/firrtl/firrtl_net.h"
#include "shell/shell.h"

namespace eda::shell {

struct ReadFirrtlCommand final : public UtopiaCommandBase<ReadFirrtlCommand> {
  ReadFirrtlCommand(): UtopiaCommandBase(
      "read_firrtl", "Reads a design from a FIRRTL file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_ERROR_IF_DESIGN(interp);
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_SHELL_ERROR_IF_NO_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    UTOPIA_SHELL_ERROR_IF_FILE_NOT_EXIST(interp, fileName);

    const auto typeIDs = eda::gate::translator::getNetlist(fileName);
    UTOPIA_SHELL_ERROR_IF(interp, typeIDs.empty(),
        "received empty list");

    UTOPIA_SHELL_ERROR_IF(interp, !setDesign(typeIDs.front(), logger),
        "validation checks failed");

    return TCL_OK;
  }
};

} // namespace eda::shell
