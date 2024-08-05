//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/translator/firrtl.h"
#include "gate/translator/yosys_converter_firrtl.h"
#include "shell/shell.h"

struct VerilogToFirCommand final :
    public UtopiaCommandBase<VerilogToFirCommand> {
  VerilogToFirCommand(): UtopiaCommandBase(
      "verilog_to_fir", "Translates Verilog to FIRRTL") {
    app.add_flag("--debug", debugMode);
    app.add_option("--top", topModule);
    app.add_option("-o, --out", outputFile);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_ERROR_IF_NO_FILES(interp, app);

    for (const auto &fileName: app.remaining()) {
      UTOPIA_ERROR_IF_FILE_NOT_EXIST(interp, fileName);
    }

    FirrtlConfig cfg;
    cfg.debugMode = debugMode;
    cfg.outputFileName = outputFile;
    cfg.topModule = topModule;
    cfg.files = app.remaining();

    YosysConverterFirrtl converter(cfg);

    return TCL_OK;
  } 

  std::string outputFile;
  std::string topModule;
  bool debugMode{false};
};
