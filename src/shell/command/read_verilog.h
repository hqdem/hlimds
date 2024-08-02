//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/translator/yosys_converter_model2.h"
#include "shell/shell.h"

struct ReadVerilogCommand final : public UtopiaCommandBase<ReadVerilogCommand> {
  ReadVerilogCommand(): UtopiaCommandBase<ReadVerilogCommand>(
      "read_verilog", "Reads a design from a Verilog file") {
    app.add_option("--frontend", frontend);
    app.add_option("--top", topModule);
    app.add_flag("--debug", debugMode);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    using DesignBuilder = eda::gate::model::DesignBuilder;

    UTOPIA_ERROR_IF_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_ERROR_IF_NO_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    UTOPIA_ERROR_IF_FILE_NOT_EXIST(interp, fileName);

    if (frontend == "yosys") {
      YosysToModel2Config cfg;
      cfg.debugMode = debugMode;
      cfg.topModule = topModule;
      cfg.files = app.remaining();

      YosysConverterModel2 cvt(cfg);      
      const auto &netID = cvt.getNetID();
      if (netID == eda::gate::model::OBJ_NULL_ID) {
        return makeError(interp, "null ID received");
      }

      designBuilder = std::make_shared<DesignBuilder>(netID);
      return TCL_OK;
    }

    return makeError(interp, fmt::format("unknown frontend '{}'", frontend));
  }
 
  std::string frontend = "yosys";
  std::string topModule;
  bool debugMode = false;
};
