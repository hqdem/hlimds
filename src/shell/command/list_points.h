//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "shell/shell.h"

namespace eda::shell {

struct ListPointsCommand final : public UtopiaCommand {
  ListPointsCommand(): UtopiaCommand(
      "list_points", "Lists the design checkpoints") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_ERROR_IF_NO_DESIGN(interp);

    const auto points = getDesign()->getPoints();

    if (points.empty()) {
      UTOPIA_SHELL_OUT << "  <empty>" << std::endl << std::flush;
      return TCL_OK;
    }

    for (const auto &point : points) {
      UTOPIA_SHELL_OUT << "  - " << point << std::endl;
    }
    UTOPIA_SHELL_OUT << std::flush;

    return TCL_OK;
  }
};

} // namespace eda::shell
