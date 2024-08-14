//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/validator.h"
#include "shell/command/delete_design.h"
#include "shell/command/exit.h"
#include "shell/command/goto_point.h"
#include "shell/command/help.h"
#include "shell/command/lec.h"
#include "shell/command/list_points.h"
#include "shell/command/logopt.h"
#include "shell/command/read_firrtl.h"
#include "shell/command/read_graphml.h"
#include "shell/command/read_liberty.h"
#include "shell/command/read_verilog.h"
#include "shell/command/save_point.h"
#include "shell/command/set_name.h"
#include "shell/command/stat_design.h"
#include "shell/command/stat_logdb.h"
#include "shell/command/techmap.h"
#include "shell/command/verilog_to_fir.h"
#include "shell/command/version.h"
#include "shell/command/write_design.h"
#include "shell/shell.h"
#include "util/env.h"

namespace eda::shell {

namespace model = eda::gate::model;
namespace optimizer = eda::gate::optimizer;

static optimizer::DesignBuilderPtr designBuilder{nullptr};

optimizer::DesignBuilderPtr getDesign() {
  return designBuilder;
}

bool setDesign(const model::CellTypeID typeID, diag::Logger &logger) {
  if (!model::validateCellType(typeID, logger))
    return false;

  const auto &type = model::CellType::get(typeID);
  if (!type.isNet() && !type.isSubnet())
    return false;

  if (type.isNet()) {
    const auto netID = type.getNetID();
    designBuilder = std::make_shared<model::DesignBuilder>(netID);
  } else {
    const auto subnetID = type.getSubnetID();
    designBuilder = std::make_shared<model::DesignBuilder>(subnetID);
  }

  return true;
}

bool setDesign(const model::NetID netID, diag::Logger &logger) {
  if (!model::validateNet(netID, logger))
    return false;

  designBuilder = std::make_shared<model::DesignBuilder>(netID);
  return true;
}

bool setDesign(const model::SubnetID subnetID, diag::Logger &logger) {
  if (!model::validateSubnet(subnetID, logger))
    return false;

  designBuilder = std::make_shared<model::DesignBuilder>(subnetID);
  return true;
}

void deleteDesign() {
  designBuilder = nullptr;
}

static int printUtopiaFile(Tcl_Interp *interp, const std::string &fileName) {
  const char *utopiaHome = std::getenv("UTOPIA_HOME");
  if (!utopiaHome) {
    return makeError(interp, "UTOPIA_HOME has not been set");
  }

  std::string filePath = std::string(utopiaHome) + "/" + fileName;
  return printFile(interp, filePath);
}

UtopiaShell::UtopiaShell() {
  addCommand(&DeleteDesignCommand::get());
  addCommand(&GotoPointCommand::get());
  addCommand(&ExitCommand::get());
  addCommand(&HelpCommand::get());
  addCommand(&LecCommand::get());
  addCommand(&ListPointsCommand::get());
  addCommand(&LogOptCommand::get());
  addCommand(&ReadFirrtlCommand::get());
  addCommand(&ReadGraphMlCommand::get());
  addCommand(&ReadLibertyCommand::get());
  addCommand(&ReadVerilogCommand::get());
  addCommand(&SavePointCommand::get());
  addCommand(&SetNameCommand::get());
  addCommand(&StatDesignCommand::get());
  addCommand(&StatLogDbCommand::get());
  addCommand(&TechMapCommand::get());
#ifdef UTOPIA_SHELL_ENABLE_VERILOG_TO_FIR
  addCommand(&VerilogToFirCommand::get());
#endif // UTOPIA_SHELL_ENABLE_VERILOG_TO_FIR
  addCommand(&VersionCommand::get());
  addCommand(&WriteDebugCommand::get());
  addCommand(&WriteDotCommand::get());
  addCommand(&WriteVerilogCommand::get());
}

void UtopiaShell::printTitle(Tcl_Interp *interp) {
  printUtopiaFile(interp, "config/title.txt");
  printNewline();
}

} // namespace eda::shell

int umain(eda::shell::UtopiaShell &shell, int argc, char **argv) {
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
  Tcl_AppInitProc *appInitProc = shell.getAppInitProc();

  if (appInitProc(interp) == TCL_ERROR) {
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
    Tcl_MainEx(argc, argv, appInitProc, interp);
  }

  Tcl_DeleteInterp(interp);
  Tcl_Finalize();
  return rc;
}

int umain(int argc, char **argv) {
  return umain(eda::shell::UtopiaShell::get(), argc, argv);
}
