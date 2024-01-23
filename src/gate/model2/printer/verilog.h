//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/printer/printer.h"
#include "util/singleton.h"

namespace eda::gate::model {

/// Prints nets in Verilog.
class VerilogPrinter final : public ModelPrinter,
                             public util::Singleton<VerilogPrinter> {
  friend class util::Singleton<VerilogPrinter>;

  VerilogPrinter(): ModelPrinter({{Pass::CELL, 0}, {Pass::CELL, 1}}) {}

  void onModelBegin(std::ostream &out, const std::string &name) override {
    out << "module " << name;
  }

  void onModelEnd(std::ostream &out, const std::string &name) override {
    out << "endmodule" << " // module " << name << "\n";
  }

  void onInterfaceBegin(std::ostream &out) override {
    out << "(\n";
    isFirstPort = true;
  }

  void onInterfaceEnd(std::ostream &out) override {
    out << "\n);\n";
  }

  void onPort(std::ostream &out, const CellInfo &cellInfo) override {
    if (!isFirstPort) {
      out << ",\n";
    }

    out << "  " << (cellInfo.type.get().isIn() ? "input" : "output") << " ";
    out << PortInfo(cellInfo, 0).getName();

    isFirstPort = false;
  }

  void onCell(std::ostream &out,
              const CellInfo &cellInfo,
              const LinksInfo &linksInfo,
              unsigned pass) override {
    const CellType &type = cellInfo.type.get();

    if (pass == 0) {
      // Declare the cell's output wires.
      if (!type.isIn() && !type.isOut()) {
        for (uint16_t port = 0; port < type.getOutNum(); ++port) {
          out << "  wire " << PortInfo(cellInfo, port).getName() << ";\n";
        }
      }
    } else {
      // Instantiate the cell.
      if (type.isIn()) {
        // Do nothing.
      } else if (type.isOut()) {
        out << "  assign " << PortInfo(cellInfo, 0).getName() << " = "
            << (linksInfo.front().inv ? "~" : "")
            << linksInfo.front().getSourceName() << ";\n";
      } else {
        out << "  " << cellInfo.getType() << "(";

        bool comma = false;
        for (uint16_t port = 0; port < type.getOutNum(); ++port) {
          if (comma) out << ", ";
          out << PortInfo(cellInfo, port).getName();
          comma = true;
        }

        for (auto linkInfo : linksInfo) {
          if (comma) out << ", ";
          out << (linkInfo.inv ? "~" : "")
              << linkInfo.getSourceName();
          comma = true;
        }

        out << ");\n";
      }
    }
  }

  bool isFirstPort = false;
};

} // namespace eda::gate::model
