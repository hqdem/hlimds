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

#include <sstream>

namespace eda::gate::model {

/// Prints nets in Verilog.
class VerilogPrinter final : public NetPrinter,
                             public util::Singleton<VerilogPrinter> {
  friend class util::Singleton<VerilogPrinter>;

  static std::string getType(const CellID &cellID) {
    return Cell::get(cellID).getType().getName();
  }

  static std::string getName(const CellID &cellID) {
    std::stringstream ss;

    ss << getType(cellID);
    ss << "_";
    ss << Cell::makeSID(cellID);

    return ss.str();
  }

  static std::string getWire(const CellID &cellID, uint16_t port) {
    const CellType &type = Cell::get(cellID).getType();

    if (type.getOutNum() <= 1) {
      return getName(cellID);
    }

    std::stringstream ss;
    ss << getName(cellID) << "_" << port;

    return ss.str();
  }

  static std::string getWire(const LinkEnd &source) {
    return getWire(source.getCellID(), source.getPort());
  }

  VerilogPrinter(): NetPrinter({{Pass::CELL, 0}, {Pass::CELL, 1}}) {}

  void onNetBegin(
      std::ostream &out, const Net &net, const std::string &name) override {
    out << "module " << name;
  }

  void onNetEnd(
      std::ostream &out, const Net &net, const std::string &name) override {
    out << "endmodule" << " // module " << name << "\n";
  }

  void onInterfaceBegin(std::ostream &out) override {
    out << "(\n";
    isFirstPort = true;
  }

  void onInterfaceEnd(std::ostream &out) override {
    out << "\n);\n";
  }

  void onPort(std::ostream &out, const CellID &cellID) override {
    if (!isFirstPort) {
      out << ",\n";
    }

    out << "  " << (Cell::get(cellID).isIn() ? "input" : "output") << " ";
    out << getWire(cellID, 0);

    isFirstPort = false;
  }

  void onDefinitionBegin(std::ostream &out) override {
    // Do nothing.
  }

  void onDefinitionEnd(std::ostream &out) override {
    // Do nothing.
  }

  void onCell(std::ostream &out, const CellID &cellID, unsigned pass) override {
    const Cell &cell = Cell::get(cellID);
    const Cell::LinkList &links = cell.getLinks();
    const CellType &type = cell.getType();

    if (pass == 0) {
      // Declare the cell's output wires.
      if (!cell.isIn() && !cell.isOut()) {
        for (uint16_t port = 0; port < type.getOutNum(); ++port) {
          out << "  wire " << getWire(cellID, port) << ";\n";
        }
      }
    } else {
      // Instantiate the cell.
      if (cell.isIn()) {
        // Do nothing.
      } else if (cell.isOut()) {
        out << "  assign " << getWire(cellID, 0) << " = "
            << getWire(links.front()) << ";\n"; 
      } else {
        out << "  " << getType(cellID) << "(";

        bool comma = false;
        for (uint16_t port = 0; port < type.getOutNum(); ++port) {
          if (comma) out << ", ";
          out << getWire(cellID, port);
          comma = true;
        }

        for (auto link : links) {
          if (comma) out << ", ";
          out << getWire(link);
          comma = true;
        }

        out << ");\n";
      }
    }
  }

  void onLink(std::ostream &out, const Link &link, unsigned pass) override {
    // Do nothing.
  }

  bool isFirstPort = false;
};

} // namespace eda::gate::model
