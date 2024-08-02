//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/printer/printer.h"
#include "shell/shell.h"

#include <ostream>

static inline void printDesign(
    std::ostream &out,
    eda::gate::model::DesignBuilder &designBuilder,
    eda::gate::model::ModelPrinter &printer) {
  using Net = eda::gate::model::Net;

  const auto &net = Net::get(designBuilder.make());
  const auto designName = designBuilder.getName();

  printer.print(out, net, designName);
}

static inline void printSubnet(
    std::ostream &out,
    eda::gate::model::DesignBuilder &designBuilder,
    size_t i,
    eda::gate::model::ModelPrinter &printer) {
  using Subnet = eda::gate::model::Subnet;

  const auto subnetID = designBuilder.getSubnetID(i);
  const auto &subnet = Subnet::get(subnetID);

  const auto subnetName = fmt::format("{}_{}", designBuilder.getName(), i);
  printer.print(out, subnet, subnetName);
}

static inline bool createDirectories(const std::string &dir) {
  if (std::filesystem::exists(dir)) {
    return true;
  }
  std::error_code error;
  return std::filesystem::create_directories(dir, error);
}

struct WriteDesignCommand : public UtopiaCommand {
  using ModelPrinter = eda::gate::model::ModelPrinter;
  using Format = ModelPrinter::Format;

  WriteDesignCommand(const char *name, const char *desc):
      UtopiaCommand(name, desc) {
    subnetOption = app.add_option("--subnet", subnetIndex, "Subnet index");
    app.allow_extras();
  }

  void setFormat(const Format format) {
    printer = &ModelPrinter::getPrinter(format);
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    assert(printer);

    UTOPIA_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_PARSE_ARGS(interp, app, argc, argv);
    UTOPIA_ERROR_IF_NO_INPUT_FILES(interp, app);

    const std::string fileName = app.remaining().at(0);
    std::filesystem::path filePath = fileName;

    UTOPIA_ERROR_IF(interp, !filePath.has_filename(),
        "path does not contain a file name");

    const std::string dir = filePath.remove_filename();
    UTOPIA_ERROR_IF(interp, !createDirectories(dir),
        fmt::format("cannot create directory '{}'", dir));

    std::ofstream out(fileName);

    if (subnetOption->count() == 0) {
      printDesign(out, *designBuilder, *printer);
      return TCL_OK;
    }
    if (subnetIndex < designBuilder->getSubnetNum()) {
      printSubnet(out, *designBuilder, subnetIndex, *printer);
      return TCL_OK;
    }

    return makeError(interp,
        fmt::format("subnet {} does not exist", subnetIndex));
  }

  ModelPrinter *printer;
  CLI::Option *subnetOption;
  size_t subnetIndex;
};

struct WriteDebugCommand final :
    public UtopiaCommandBase<WriteDebugCommand, WriteDesignCommand> {
  WriteDebugCommand():
      UtopiaCommandBase<WriteDebugCommand, WriteDesignCommand>(
          "write_debug", "Writes the design to a debug file") {
    setFormat(Format::SIMPLE);
  }
};

struct WriteDotCommand final :
    public UtopiaCommandBase<WriteDotCommand, WriteDesignCommand> {
  WriteDotCommand():
      UtopiaCommandBase<WriteDotCommand, WriteDesignCommand>(
          "write_dot", "Writes the design to a DOT file") {
    setFormat(Format::DOT);
  }
};

struct WriteVerilogCommand final :
    public UtopiaCommandBase<WriteVerilogCommand, WriteDesignCommand> {
  WriteVerilogCommand():
      UtopiaCommandBase<WriteVerilogCommand, WriteDesignCommand>(
          "write_verilog", "Writes the design to a Verilog file") {
    setFormat(Format::VERILOG);
  }
};


