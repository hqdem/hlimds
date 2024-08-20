//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/validator.h"
#include "gate/translator/model2.h"
#include "gate/translator/yosys_converter_model2.h"
#include "shell/shell.h"

namespace eda::shell {

struct ReadVerilogCommand final : public UtopiaCommandBase<ReadVerilogCommand> {
  ReadVerilogCommand(): UtopiaCommandBase(
      "read_verilog", "Reads a design from a Verilog file") {
    app.add_option("--frontend", frontend);
    app.add_option("--top", topModule);
    app.add_flag("--debug", debugMode);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    namespace model = eda::gate::model;
    namespace translator = eda::gate::translator;

    UTOPIA_SHELL_ERROR_IF_DESIGN(interp);
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_SHELL_ERROR_IF_NO_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    UTOPIA_SHELL_ERROR_IF_FILE_NOT_EXIST(interp, fileName);

    UTOPIA_SHELL_ERROR_IF(interp, (frontend != "rtlil"),
        fmt::format("unknown frontend '{}'", frontend)); 

    model::CellTypeID typeID{model::OBJ_NULL_ID};
    if (frontend == "rtlil" ) {
      typeID = translator::readVerilogDesign(topModule, app.remaining());
    }

    UTOPIA_SHELL_ERROR_IF(interp, !setDesign(typeID, logger),
      "validation checks failed");

    return TCL_OK;
  }
 
  std::string frontend = "rtlil";
  std::string topModule;
  bool debugMode = false;
};

} // namespace eda::shell
