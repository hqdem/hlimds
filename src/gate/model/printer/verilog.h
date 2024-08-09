//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/printer/printer.h"
#include "util/singleton.h"

namespace eda::gate::model {

/// Prints nets in Verilog.
class VerilogPrinter final : public ModelPrinter,
                             public util::Singleton<VerilogPrinter> {
  friend class util::Singleton<VerilogPrinter>;

  static constexpr enum { MODULE, UDP } MajMethod = MODULE;

  VerilogPrinter(): ModelPrinter({{Pass::CELL, 0}, {Pass::CELL, 1}}) {}

  void onModelBegin(std::ostream &out, const std::string &name) override {
    // MAJ gates are not supported in Verilog and handled in a special way.
    if constexpr (MajMethod == UDP) {
      out << "primitive maj(output out, input x, input y, input z);\n";
      out << "  table\n";
      out << "    // x y z   out\n";
      out << "       0 0 0 : 0;\n";
      out << "       0 0 1 : 0;\n";
      out << "       0 1 0 : 0;\n";
      out << "       0 1 1 : 1;\n";
      out << "       1 0 0 : 0;\n";
      out << "       1 0 1 : 1;\n";
      out << "       1 1 0 : 1;\n";
      out << "       1 1 1 : 1;\n";
      out << "  endtable\n";
      out << "endprimitive // primitive maj\n";
    } else {
      out << "module maj(output out, input x, input y, input z);\n";
      out << "  assign out = (x & y) | (x & z) | (y & z);\n";
      out << "endmodule // module maj\n";
    }
    out << "\n";
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
    if (type.isGate() && !type.isMaj())
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

  void declareWiresForCellOutputs(std::ostream &out, const CellInfo &cellInfo) {
    const auto &type = cellInfo.type.get();

    if (type.isIn() || type.isOut())
      return;

    for (uint16_t output = 0; output < type.getOutNum(); ++output) {
      printIndent(out, 1);
      out << "wire " << PortInfo(cellInfo, output).getName() << ";\n";
    }
  }

  void assignConstant(std::ostream &out, const CellInfo &cellInfo) {
    const auto &type = cellInfo.type.get();
    assert(type.isZero() || type.isOne());

    printIndent(out, 1);
    out << "assign " << PortInfo(cellInfo, 0).getName()
        << " = " << (type.isZero() ? "0" : "1") << ";\n";
  }

  void defineInputBinding(std::ostream &out, const LinkInfo &linkInfo) {
    out << getLinkExpr(linkInfo);
  }

  void defineInputBinding(std::ostream &out,
                    const LinksInfo &linksInfo,
                    size_t index,
                    size_t width) {
    assert(width > 0);

    if (width == 1) {
      defineInputBinding(out, linksInfo[index]);
    } else {
      out << "{";
      bool comma = false;
      for (size_t i = 0; i < width; ++i) {
        if (comma) out << ", ";
        defineInputBinding(out, linksInfo[index + width - 1 - i]);
        comma = true;
      }
      out << "}";
    }
  }

  void defineOutputBinding(std::ostream &out, const PortInfo &portInfo) {
    out << portInfo.getName();
  }

  void defineOutputBinding(std::ostream &out,
                     const CellInfo &cellInfo,
                     size_t index,
                     size_t width) {
    assert(width > 0);

    if (width == 1) {
      defineOutputBinding(out, PortInfo(cellInfo, index));
    } else {
      out << "{";
      bool comma = false;
      for (size_t i = 0; i < width; ++i) {
        if (comma) out << ", ";
        defineOutputBinding(out, PortInfo(cellInfo, index + width - 1 - i));
        comma = true;
      }
      out << "}";
    }
  }

  void instantiateCell(std::ostream &out,
                       const CellInfo &cellInfo,
                       const LinksInfo &linksInfo) {
    const auto &type = cellInfo.type.get();
    assert(!type.isIn() && !type.isOut());

    printIndent(out, 1);

    out << cellInfo.getType() << " " << getInstanceName(cellInfo) << "(";
    bool comma = false;

    if (type.isGate()) {
      assert(!type.isMaj() || linksInfo.size() == 3);

      // In built-in Verilog gates, outputs come before inputs.
      for (uint16_t output = 0; output < type.getOutNum(); ++output) {
        if (comma) out << ", ";
        defineOutputBinding(out, PortInfo(cellInfo, output));
        comma = true;
      }

      for (auto linkInfo : linksInfo) {
        if (comma) out << ", ";
        defineInputBinding(out, linkInfo);
        comma = true;
      }
    } else {
      assert(type.hasAttr() && type.getAttr().hasPortInfo());

      // In custom gates, the order of ports can be arbitrary.
      const auto &attr = type.getAttr();
      const auto ports = attr.getOrderedPorts();

      size_t input{0}, output{0};
      for (const auto &port : ports) {
        if (comma) out << ", ";
        if (port.input) {
          defineInputBinding(out, linksInfo, input, port.width);
          input += port.width;
        } else {
          defineOutputBinding(out, cellInfo, output, port.width);
          output += port.width;
        }
        comma = true;
      }
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
    out << std::string(2 * n, ' ');
  }

  bool isFirstPort = false;
};

} // namespace eda::gate::model
