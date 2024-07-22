//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "CLI/App.hpp"
#include "config.h"

#include "easylogging++.h"

#include <tcl.h>

INITIALIZE_EASYLOGGINGPP

extern Tcl_AppInitProc Utopia_TclInit;

int main(int argc, char **argv) {
    START_EASYLOGGINGPP(argc, argv);

    std::stringstream title;
    std::stringstream version;

    version << VERSION_MAJOR << "." << VERSION_MINOR;

    title << "Utopia EDA " << version.str() << " | ";
    title << "Copyright (C) " << YEAR_STARTED << "-" << YEAR_CURRENT << " ISP RAS";

    CLI::App app{"Utopia EDA"};

    std::string path = "";
    app.add_option("-s, --script", path, "Path to Tcl script file");
    app.allow_extras();

    CLI11_PARSE(app, argc, argv);

    Tcl_FindExecutable(argv[0]);
    Tcl_Interp *interp = Tcl_CreateInterp();

    if (!path.empty()) {
      std::vector<std::string> script_args = app.remaining();
      const char *fileName = path.c_str();

      Tcl_Obj *tcl_argv0 = Tcl_NewStringObj(fileName, -1);
      Tcl_SetVar2Ex(interp, "argv0", nullptr, tcl_argv0, TCL_GLOBAL_ONLY);

      Tcl_Obj *tcl_argv_list = Tcl_NewListObj(0, nullptr);
      for (const auto &arg : script_args) {
        Tcl_ListObjAppendElement(
            interp,
            tcl_argv_list,
            Tcl_NewStringObj(arg.c_str(), -1));
      }

      Tcl_Obj *tcl_argc =
          Tcl_NewLongObj(static_cast<long>(script_args.size()));
      Tcl_SetVar2Ex(interp, "argc", nullptr, tcl_argc, TCL_GLOBAL_ONLY);
      Tcl_SetVar2Ex(interp, "argv", nullptr, tcl_argv_list, TCL_GLOBAL_ONLY);

      if ((Utopia_TclInit(interp) == TCL_ERROR) ||
          (Tcl_EvalFile(interp, fileName) == TCL_ERROR)) {
        std::cout << Tcl_GetStringResult(interp) << std::endl;
        return 1;
      }
    } else {
      std::cout << title.str() << std::endl;
      Tcl_Main(argc, argv, Utopia_TclInit);
    }

    Tcl_DeleteInterp(interp);
    Tcl_Finalize();
    return 0;
}
