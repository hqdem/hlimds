//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "config.h"
#include "shell/shell.h"

struct VersionCommand final : public UtopiaCommandBase<VersionCommand> {
  VersionCommand(): UtopiaCommandBase(
      "version", "Prints Utopia EDA version") {}

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_OUT << "Utopia EDA "
               << "version "
               << VERSION_MAJOR
               << "."
               << VERSION_MINOR
               << std::endl
               << std::flush;

    return TCL_OK;
  } 
};
