//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "umain.h"
#include "context/utopia_context.h"
#include "diag/logger.h"
#include "shell/shell.h"

int umain(eda::shell::UtopiaShell &shell,
          eda::context::UtopiaContext &context,
          int argc, char **argv) {
  UTOPIA_INITIALIZE_LOGGER();

  shell.setContext(&context);

  CLI::App app{shell.getName()};

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
  //Tcl_AppInitProc *appInitProc = shell.getAppInitProc();

  if (shell.appInitProc(interp) == TCL_ERROR) {
    UTOPIA_SHELL_ERR << "Failed to initialize a Tcl interpreter" << std::endl;
    return 1;
  }

  shell.printTitle(interp);

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
      UTOPIA_SHELL_ERR << Tcl_GetStringResult(interp) << std::endl;
      rc = 1;
    }
    exitAfterEval = true;
  } else if (evalMode->count()) {
    if (Tcl_Eval(interp, script.c_str()) == TCL_ERROR) {
      UTOPIA_SHELL_ERR << Tcl_GetStringResult(interp) << std::endl;
      rc = 1;
    }
    exitAfterEval = true;
  }
  if (interactiveMode || !exitAfterEval) {
    Tcl_MainEx(argc, argv, 
              [](Tcl_Interp *interp) -> int { return TCL_OK; }, interp);
  }

  Tcl_DeleteInterp(interp);
  Tcl_Finalize();
  return rc;
}

int umain(int argc, char **argv) {
  eda::shell::UtopiaShell shell;
  eda::context::UtopiaContext context;
  return umain(shell, context, argc, argv);
}