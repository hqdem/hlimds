//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "shell/shell.h"

#include <easylogging++.h>

INITIALIZE_EASYLOGGINGPP

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);
  return Utopia_Main(argc, argv);
}
