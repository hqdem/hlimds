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

static inline const eda::gate::model::Subnet &getSubnet(
    eda::gate::model::DesignBuilder &designBuilder, size_t i) {
  const auto subnetID = designBuilder.getSubnetID(i);
  return eda::gate::model::Subnet::get(subnetID);
}

static inline bool isTrivialSubnet(
    eda::gate::model::DesignBuilder &designBuilder, size_t i) {
  const auto &subnet = getSubnet(designBuilder, i);
  return subnet.getInNum() == 1
      && subnet.getOutNum() == 1
      && subnet.getCellNum() == 2;
}
 
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

  const auto &subnet = getSubnet(designBuilder, i);
  const auto subnetName = fmt::format("{}_{}", designBuilder.getName(), i);

  model::print(out, format, subnetName, subnet, model::OBJ_NULL_ID);
}

struct WriteDesignCommand : public UtopiaCommand {
  using Format = eda::gate::model::Format;

  WriteDesignCommand(const char *name, const char *desc):
      UtopiaCommand(name, desc) {
    subnetIndexOption = app.add_option(
        "--subnet-index", subnetIndex, "Subnet index");
    subnetSplitOption = app.add_flag(
        "--subnet-split", "Printing all subnets");
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

    if (subnetSplitOption->count() == 0 && subnetIndexOption->count() != 0) {
      UTOPIA_SHELL_ERROR_IF(interp, subnetIndex >= getDesign()->getSubnetNum(),
           fmt::format("subnet {} does not exist", subnetIndex));
    }

    const std::string dir = filePath.remove_filename();
    if (!dir.empty()) {
      UTOPIA_SHELL_ERROR_IF(interp, !createDirectories(dir),
          fmt::format("cannot create directory '{}'", dir));
    }

    if (subnetSplitOption->count() == 0) {
      std::ofstream out(fileName);

      if (subnetIndexOption->count() == 0) {
        printDesign(out, format, *getDesign());
      } else {
        printSubnet(out, format, *getDesign(), subnetIndex);
      }
    } else {
      for (size_t i = 0; i < getDesign()->getSubnetNum(); ++i) {
        if (isTrivialSubnet(*getDesign(), i)) continue;

        std::filesystem::path oldFilePath = fileName;

        const std::string oldExt = oldFilePath.extension();
        const std::string newExt = fmt::format("{:05}{}", i, oldExt);

        std::ofstream out(oldFilePath.replace_extension(newExt));
        printSubnet(out, format, *getDesign(), i);
      }
    }

    return TCL_OK;
  }

  Format format{eda::gate::model::DEBUG};
  CLI::Option *subnetIndexOption;
  CLI::Option *subnetSplitOption;
  size_t subnetIndex;
};

struct WriteDebugCommand final : public WriteDesignCommand {
  WriteDebugCommand(): WriteDesignCommand(
          "write_debug", "Writes the design to a debug file") {
    setFormat(eda::gate::model::DEBUG);
  }
};

struct WriteDotCommand final : public WriteDesignCommand {
  WriteDotCommand(): WriteDesignCommand(
          "write_dot", "Writes the design to a DOT file") {
    setFormat(eda::gate::model::DOT);
  }
};

struct WriteVerilogCommand final : public WriteDesignCommand {
  WriteVerilogCommand(): WriteDesignCommand(
          "write_verilog", "Writes the design to a Verilog file") {
    setFormat(eda::gate::model::VERILOG);
  }
};

} // namespace eda::shell
