//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/printer/net_printer.h"
#include "shell/shell.h"

#include <ostream>

namespace eda::shell {

static inline void printDesign(
    std::ostream &out,
    eda::gate::model::Format format,
    eda::gate::model::DesignBuilder &designBuilder) {
  namespace model = eda::gate::model;

  const auto &net = model::Net::get(designBuilder.make());
  const auto designName = designBuilder.getName();

  model::print(out, format, designName, net, designBuilder.getTypeID());
}

static inline void printSubnet(
    std::ostream &out,
    eda::gate::model::Format format,
    eda::gate::model::DesignBuilder &designBuilder,
    size_t i) {
  namespace model = eda::gate::model;

  const auto subnetID = designBuilder.getSubnetID(i);
  const auto &subnet = model::Subnet::get(subnetID);

  const auto subnetName = fmt::format("{}_{}", designBuilder.getName(), i);
  model::print(out, format, subnetName, subnet, model::OBJ_NULL_ID);
}

struct WriteDesignCommand : public UtopiaCommand {
  using Format = eda::gate::model::Format;

  WriteDesignCommand(const char *name, const char *desc):
      UtopiaCommand(name, desc) {
    subnetOption = app.add_option("--subnet", subnetIndex, "Subnet index");
    app.allow_extras();
  }

  void setFormat(const Format format) {
    this->format = format;
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    UTOPIA_SHELL_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_SHELL_ERROR_IF_NO_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    std::filesystem::path filePath = fileName;

    UTOPIA_SHELL_ERROR_IF(interp, !filePath.has_filename(),
        "path does not contain a file name");

    if (subnetOption->count() != 0) {
      UTOPIA_SHELL_ERROR_IF(interp, subnetIndex >= getDesign()->getSubnetNum(),
           fmt::format("subnet {} does not exist", subnetIndex));
    }

    const std::string dir = filePath.remove_filename();
    if (!dir.empty()) {
      UTOPIA_SHELL_ERROR_IF(interp, !createDirectories(dir),
          fmt::format("cannot create directory '{}'", dir));
    }

    std::ofstream out(fileName);
    if (subnetOption->count() == 0) {
      printDesign(out, format, *getDesign());
    } else {
      printSubnet(out, format, *getDesign(), subnetIndex);
    }

    return TCL_OK;
  }

  Format format{eda::gate::model::DEBUG};
  CLI::Option *subnetOption;
  size_t subnetIndex;
};

struct WriteDebugCommand final :
    public UtopiaCommandBase<WriteDebugCommand, WriteDesignCommand> {
  WriteDebugCommand(): UtopiaCommandBase(
          "write_debug", "Writes the design to a debug file") {
    setFormat(eda::gate::model::DEBUG);
  }
};

struct WriteDotCommand final :
    public UtopiaCommandBase<WriteDotCommand, WriteDesignCommand> {
  WriteDotCommand(): UtopiaCommandBase(
          "write_dot", "Writes the design to a DOT file") {
    setFormat(eda::gate::model::DOT);
  }
};

struct WriteVerilogCommand final :
    public UtopiaCommandBase<WriteVerilogCommand, WriteDesignCommand> {
  WriteVerilogCommand(): UtopiaCommandBase(
          "write_verilog", "Writes the design to a Verilog file") {
    setFormat(eda::gate::model::VERILOG);
  }
};

} // namespace eda::shell
