//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "config.h"

#include "CLI/App.hpp"
#include "easylogging++.h"
#include <fmt/format.h>
#include <tcl.h>

#include <iostream>

#define UTOPIA_OUT std::cout

INITIALIZE_EASYLOGGINGPP

extern Tcl_AppInitProc Utopia_TclInit;

static inline void printNewline() {
  UTOPIA_OUT << std::endl;
}

static int printFile(Tcl_Interp *interp, const std::string &fileName) {
  const char *utopiaHome = std::getenv("UTOPIA_HOME");
  if (!utopiaHome) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(
        "UTOPIA_HOME has not been set", -1));
    return TCL_ERROR;
  }

  std::string filePath = std::string(utopiaHome) + "/" + fileName;
  std::ifstream file(filePath);

  if (!file.is_open()) {
    Tcl_SetObjResult(interp, Tcl_NewStringObj(
      fmt::format("unable to open file '{}'", filePath).c_str(), -1));
    return TCL_ERROR;
  }

  std::string line;
  while (getline(file, line)) {
    UTOPIA_OUT << line << std::endl;
  }

  file.close();
  return TCL_OK;
}

static inline int printTitle(Tcl_Interp *interp) {
  return printFile(interp, "doc/help/Title.txt");
}

static inline int printCopyright(Tcl_Interp *interp) {
  return printFile(interp, "doc/help/Copyright.txt");
}

void printTitleCopyright(Tcl_Interp *interp) {
  printNewline();
  printTitle(interp);
  printNewline();
  printCopyright(interp);
  printNewline();
}

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  CLI::App app{"Utopia EDA"};

  std::string path = "";
  std::string script = "";
  bool interactiveMode = false;
  bool exitAfterEval = false;
  auto *fileMode = app.add_option(
      "-s, --script",
      path,
      "Executes a TCL script from a file");
  auto *evalMode = app.add_option(
      "-e, --evaluate",
      script,
      "Executes a TCL script from the terminal");
  app.add_flag(
      "-i, --interactive",
      interactiveMode,
      "Enters to interactive mode");
  app.allow_extras();

  CLI11_PARSE(app, argc, argv);

  Tcl_FindExecutable(argv[0]);
  Tcl_Interp *interp = Tcl_CreateInterp();
  if (Utopia_TclInit(interp) == TCL_ERROR) {
    std::cerr << "Failed to init Tcl interpreter\n";
    return 1;
  }
  printTitleCopyright(interp);

  int rc = 0;
  if (fileMode->count()) {
    std::vector<std::string> scriptArgs = app.remaining();
    const char *fileName = path.c_str();

    Tcl_Obj *tclArgv0 = Tcl_NewStringObj(fileName, -1);
    Tcl_SetVar2Ex(interp, "argv0", nullptr, tclArgv0, TCL_GLOBAL_ONLY);

    Tcl_Obj *tclArgvList = Tcl_NewListObj(0, nullptr);
    for (const auto &arg : scriptArgs) {
      Tcl_ListObjAppendElement(
          interp,
          tclArgvList,
          Tcl_NewStringObj(arg.c_str(), -1));
    }

    Tcl_Obj *tclArgc =
        Tcl_NewLongObj(static_cast<long>(scriptArgs.size()));
    Tcl_SetVar2Ex(interp, "argc", nullptr, tclArgc, TCL_GLOBAL_ONLY);
    Tcl_SetVar2Ex(interp, "argv", nullptr, tclArgvList, TCL_GLOBAL_ONLY);

    if (Tcl_EvalFile(interp, fileName) == TCL_ERROR) {
      std::cerr << Tcl_GetStringResult(interp) << '\n';
      rc = 1;
    }
    exitAfterEval = true;
  } else if (evalMode->count()) {
    if (Tcl_Eval(interp, script.c_str()) == TCL_ERROR) {
      std::cerr << Tcl_GetStringResult(interp) << '\n';
      rc = 1;
    }
    exitAfterEval = true;
  }
  if (interactiveMode || !exitAfterEval) {
    Tcl_MainEx(argc, argv, Utopia_TclInit, interp);
  }

  Tcl_DeleteInterp(interp);
  Tcl_Finalize();
  return rc;
}
