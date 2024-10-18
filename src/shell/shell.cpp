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
#include "shell/command/save_point.h"
#include "shell/command/set_name.h"
#include "shell/command/stat_design.h"
#include "shell/command/stat_logdb.h"
#include "shell/command/techmap.h"
#include "shell/command/unmap.h"
#include "shell/command/verilog_to_fir.h"
#include "shell/command/version.h"
#include "shell/command/write_dataflow.h"
#include "shell/command/write_design.h"
#include "shell/command/write_verilog_lib.h"
#include "shell/shell.h"

namespace eda::shell {

namespace model = eda::gate::model;
namespace optimizer = eda::gate::optimizer;

static std::shared_ptr<model::DesignBuilder> designBuilder{nullptr};

std::shared_ptr<model::DesignBuilder> getDesign() {
  return designBuilder;
}

bool setDesign(const model::CellTypeID typeID, diag::Logger &logger) {
  if (!model::validateCellType(typeID, logger))
    return false;

  const auto &type = model::CellType::get(typeID);
  if (!type.hasImpl())
    return false;

  designBuilder = std::make_shared<model::DesignBuilder>(typeID);
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
  addCommand(std::make_unique<DeleteDesignCommand>());
  addCommand(std::make_unique<GotoPointCommand>());
  addCommand(std::make_unique<ExitCommand>());
  addCommand(std::make_unique<HelpCommand>());
  addCommand(std::make_unique<LecCommand>());
  addCommand(std::make_unique<ListPointsCommand>());
  addCommand(std::make_unique<LogOptCommand>());
  addCommand(std::make_unique<ReadFirrtlCommand>());
  addCommand(std::make_unique<ReadGraphMlCommand>());
  addCommand(std::make_unique<ReadLibertyCommand>());
  addCommand(std::make_unique<SavePointCommand>());
  addCommand(std::make_unique<SetNameCommand>());
  addCommand(std::make_unique<StatDesignCommand>());
  addCommand(std::make_unique<StatLogDbCommand>());
  addCommand(std::make_unique<TechMapCommand>());
  addCommand(std::make_unique<UnmapCommand>());
#ifdef UTOPIA_SHELL_ENABLE_VERILOG_TO_FIR
  addCommand(std::make_unique<VerilogToFirCommand>());
#endif // UTOPIA_SHELL_ENABLE_VERILOG_TO_FIR
  addCommand(std::make_unique<VersionCommand>());
  addCommand(std::make_unique<WriteDataflowCommand>());
  addCommand(std::make_unique<WriteDebugCommand>());
  addCommand(std::make_unique<WriteDotCommand>());
  addCommand(std::make_unique<WriteLogDbCommand>());
  addCommand(std::make_unique<WriteVerilogCommand>());
  addCommand(std::make_unique<WriteVerilogLibraryCommand>());
}

void UtopiaShell::printTitle(Tcl_Interp *interp) {
  printUtopiaFile(interp, "config/title.txt");
  printNewline();
}

} // namespace eda::shell

