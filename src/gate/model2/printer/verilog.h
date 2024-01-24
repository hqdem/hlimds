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
    const auto &type = cellInfo.type.get();

    if (pass == 0) {
      declareWiresForCellOutputs(out, cellInfo);
      return;
    }
 
    if (type.isZero() || type.isOne()) {
      assignConstant(out, cellInfo);
      return;
    }

    if (!type.isIn() && !type.isOut()) {
      instantiateCell(out, cellInfo, linksInfo);
      return;
    }

    if (type.isOut()) {
      assignModelOutputs(out, cellInfo, linksInfo);
      return;
    }
}

private:
  static std::string getInstanceName(const CellInfo &cellInfo) { 
    const auto &type = cellInfo.type.get();

    // Standard logic gates do not require names.
    if (!type.isCell() && !type.isSoft() && !type.isHard())
      return "";

    // Instances of technological cells and IPs should be named.
    std::stringstream ss;

    ss << "cell_" << cellInfo.getType();
    ss << "_";
    ss << cellInfo.cell;

    return ss.str();
  }

  static std::string getLinkExpr(const LinkInfo &linkInfo) {
    std::stringstream ss;

    if (linkInfo.inv) ss << "~";
    ss << linkInfo.getSourceName();

    return ss.str();
  }

  void declareWiresForCellOutputs(
      std::ostream &out, const CellInfo &cellInfo) {
    const auto &type = cellInfo.type.get();

    if (type.isIn() || type.isOut())
      return;

    for (uint16_t output = 0; output < type.getOutNum(); ++output) {
      printIndent(out, 1);
      out << "wire " << PortInfo(cellInfo, output).getName() << ";\n";
    }
  }

  void assignConstant(
      std::ostream &out, const CellInfo &cellInfo) {
    const auto &type = cellInfo.type.get();
    assert(type.isZero() || type.isOne());

    printIndent(out, 1);
    out << "assign " << PortInfo(cellInfo, 0).getName()
        << " = " << (type.isZero() ? "0" : "1") << ";\n";
  }

  void instantiateCell(std::ostream &out,
                       const CellInfo &cellInfo,
                       const LinksInfo &linksInfo) {
    const auto &type = cellInfo.type.get();
    assert(!type.isIn() && !type.isOut());

    printIndent(out, 1);
    out << cellInfo.getType() << " " << getInstanceName(cellInfo) << "(";

    bool comma = false;
    for (uint16_t output = 0; output < type.getOutNum(); ++output) {
      if (comma) out << ", ";
      out << PortInfo(cellInfo, output).getName();
      comma = true;
    }

    for (auto linkInfo : linksInfo) {
      if (comma) out << ", ";
      out << getLinkExpr(linkInfo);
      comma = true;
    }

    out << ");\n";
  }

  void assignModelOutputs(std::ostream &out,
                          const CellInfo &cellInfo,
                          const LinksInfo &linksInfo) {
    const auto &type = cellInfo.type.get();
    assert(type.isOut());

    printIndent(out, 1);
    out << "assign " << PortInfo(cellInfo, 0).getName()
        << " = " << getLinkExpr(linksInfo.front()) << ";\n"; 
  }

  void printIndent(std::ostream &out, size_t n) {
    for (size_t i = 0; i < n; ++i) {
      out << "  ";
    }
  }

  bool isFirstPort = false;
};

} // namespace eda::gate::model
