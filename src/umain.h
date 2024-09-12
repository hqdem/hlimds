//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "context/utopia_context.h"
#include "shell/shell.h"

int umain(eda::shell::UtopiaShell &shell,
          eda::context::UtopiaContext &context,
          int argc, char **argv);
int umain(int argc, char **argv);