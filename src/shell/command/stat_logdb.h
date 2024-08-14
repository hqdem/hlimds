//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/get_dbstat.h"
#include "shell/shell.h"

namespace eda::shell {

struct StatLogDbCommand final : public UtopiaCommandBase<StatLogDbCommand> {
  StatLogDbCommand(): UtopiaCommandBase(
      "stat_logdb", "Prints information about a logopt database") {
    app.add_option("--otype", outputType)->expected(1);
    app.add_option("--out", outputFile)->expected(1);
    app.add_option("--ttsize", ttSize)->expected(1)->required(true);
    app.add_option("--tt", truthTable)->expected(1)->required(true);
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    using OutType = eda::gate::optimizer::OutType;

    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_SHELL_ERROR_IF_NO_FILES(interp, app);

    eda::gate::optimizer::NpnDbConfig config;
    config.dbPath = app.remaining().at(0);

    if (outputType == "DOT") {
      config.outType = OutType::DOT;
    } else if (outputType == "INFO") {
      config.outType = OutType::INFO;
    } else if (outputType == "BOTH") {
      config.outType = OutType::BOTH;
    } else {
      return makeError(interp,
          fmt::format("unknown output type '{}'", outputType));
    }

    config.outName = outputFile;
    config.ttSize = ttSize;
    config.binLines.push_back(truthTable);

    return getDbStat(UTOPIA_SHELL_OUT, config) ? TCL_ERROR : TCL_OK;
  }

  size_t ttSize;
  std::string outputType = "BOTH";
  std::string outputFile;
  std::string truthTable;
};

} // namespace eda::shell
