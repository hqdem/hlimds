//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/translator/graphml.h"
#include "shell/shell.h"

struct ReadGraphMlCommand final : public UtopiaCommandBase<ReadGraphMlCommand> {
  ReadGraphMlCommand(): UtopiaCommandBase(
      "read_graphml", "Reads a design from a GraphML file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    using DesignBuilder = eda::gate::model::DesignBuilder;
    using GmlTranslator = eda::gate::translator::GmlTranslator;

    UTOPIA_ERROR_IF_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_ERROR_IF_NO_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    UTOPIA_ERROR_IF_FILE_NOT_EXIST(interp, fileName);

    GmlTranslator::ParserData data;
    GmlTranslator parser;
    const auto &subnet = parser.translate(fileName, data)->make(true);
    designBuilder = std::make_shared<DesignBuilder>(subnet);

    return TCL_OK;
  }
};
