//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/validator.h"
#include "gate/translator/graphml.h"
#include "shell/shell.h"

namespace eda::shell {

struct ReadGraphMlCommand final : public UtopiaCommandBase<ReadGraphMlCommand> {
  ReadGraphMlCommand(): UtopiaCommandBase(
      "read_graphml", "Reads a design from a GraphML file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    using GmlTranslator = eda::gate::translator::GmlTranslator;

    UTOPIA_SHELL_ERROR_IF_DESIGN(interp);
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_SHELL_ERROR_IF_NO_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    UTOPIA_SHELL_ERROR_IF_FILE_NOT_EXIST(interp, fileName);

    GmlTranslator::ParserData data;
    GmlTranslator parser;
    const auto subnetID = parser.translate(fileName, data)->make(true);

    UTOPIA_SHELL_ERROR_IF(interp, !setDesign(subnetID, logger),
        "validation checks failed");

    return TCL_OK;
  }
};

} // namespace eda::shell
