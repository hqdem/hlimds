//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/translator/fir/fir_net.h"
#include "shell/shell.h"

namespace eda::shell {

struct ReadFIRRTLCommand final : public UtopiaCommandBase<ReadFIRRTLCommand> {
  ReadFIRRTLCommand(): UtopiaCommandBase(
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
    const auto netIDs = eda::gate::translator::getNet(fileName);
    for (const auto &netID : netIDs) {
      if (netID == eda::gate::model::OBJ_NULL_ID) {
        return makeError(interp, "null ID received");
      }
    }

    designBuilder = std::make_shared<DesignBuilder>(
        CellType::get(netIDs.front()).getNetID());
    return TCL_OK;
  }
};

} // namespace eda::shell
