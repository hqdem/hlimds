//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "diag/logger.h"
#include "gate/translator/fir/fir_net.h"
#include "shell/shell.h"

namespace eda::shell {

struct ReadFirrtlCommand final : public UtopiaCommandBase<ReadFirrtlCommand> {
  ReadFirrtlCommand(): UtopiaCommandBase(
      "read_firrtl", "Reads a design from a FIRRTL file") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    using DesignBuilder = eda::gate::model::DesignBuilder;

    UTOPIA_ERROR_IF_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_ERROR_IF_NO_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    UTOPIA_ERROR_IF_FILE_NOT_EXIST(interp, fileName);

    const auto typeIDs = eda::gate::translator::getNet(fileName);
    UTOPIA_ERROR_IF(interp, typeIDs.empty(),
        "received empty list");

    const auto typeID = typeIDs.front();
    UTOPIA_ERROR_IF(interp, typeID == eda::gate::model::OBJ_NULL_ID,
        "received null type");

    UTOPIA_ERROR_IF(interp, !validateCellType(typeID, logger),
        "validation checks failed");

    const auto &type = CellType::get(typeID);
    UTOPIA_ERROR_IF(interp, !type.isNet(),
        "received null net");
 
    const auto netID = type.getNetID();
    designBuilder = std::make_shared<DesignBuilder>(netID);

    return TCL_OK;
  }
};

} // namespace eda::shell
