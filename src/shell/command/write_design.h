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

static inline std::string getSubnetLocalName(size_t i) {
  return fmt::format("{:06}", i);
}

static inline std::string getSubnetGlobalName(
    eda::gate::model::DesignBuilder &designBuilder, size_t i) {
  return fmt::format("{}_{}", designBuilder.getName(), getSubnetLocalName(i));
}

static inline const eda::gate::model::Subnet &getSubnet(
    eda::gate::model::DesignBuilder &designBuilder, size_t i) {
  const auto subnetID = designBuilder.getSubnetID(i);
  return eda::gate::model::Subnet::get(subnetID);
}

static inline bool isTrivialSubnet(
    eda::gate::model::DesignBuilder &designBuilder, size_t i) {
  const auto &subnet = getSubnet(designBuilder, i);
  return subnet.isTrivial();
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
  const auto subnetName = getSubnetGlobalName(designBuilder, i);

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
    if (auto status = createParentDirs(interp, fileName); status != TCL_OK) {
      return status;
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

        const std::string number = getSubnetLocalName(i);
        const std::string oldExt = oldFilePath.extension();
        const std::string newExt = fmt::format("{}{}", number, oldExt);

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

struct WriteLogDbCommand final : public WriteDesignCommand {
  WriteLogDbCommand(): WriteDesignCommand(
      "write_logdb", "Writes the design to a LogDb file") {
    setFormat(eda::gate::model::LOGDB);
  }
};

struct WriteVerilogCommand final : public WriteDesignCommand {
  WriteVerilogCommand(): WriteDesignCommand(
      "write_verilog", "Writes the design to a Verilog file") {
    setFormat(eda::gate::model::VERILOG);
  }
};

} // namespace eda::shell
