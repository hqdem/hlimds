//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/printer/verilog.h"
#include "gate/model/printer/verilog_lib.h"

#include <cassert>
#include <sstream>

namespace eda::gate::model {

static inline void printIndent(std::ostream &out, size_t n) {
  out << std::string(2 * n, ' ');
}

static inline std::string getInstanceName(
    const ModelPrinter::CellInfo &cellInfo) { 
  const auto &type = cellInfo.type.get();

  // Standard logic gates do not require names.
  if (type.isGate() && !type.isMaj() && !type.isSeqGate())
    return "";

  // Instances of technological cells and IPs should be named.
  std::stringstream ss;

  ss << cellInfo.getType();
  ss << "_cell_";
  ss << cellInfo.cell;

  return ss.str();
}

static inline std::string getLinkExpr(
    const ModelPrinter::LinkInfo &linkInfo) {
  std::stringstream ss;

  if (linkInfo.inv) ss << "~";
  ss << linkInfo.getSourceName();

  return ss.str();
}

static inline void declareWiresForCellOutputs(
    std::ostream &out, const ModelPrinter::CellInfo &cellInfo) {
  const auto &type = cellInfo.type.get();

  if (type.isIn() || type.isOut())
    return;

  for (uint16_t output = 0; output < type.getOutNum(); ++output) {
    printIndent(out, 1);
    // Space before ";" is for escaped identifiers.
    out << "wire " << ModelPrinter::PortInfo(cellInfo, output).getName() << " ;\n";
  }
}

static inline void assignConstant(
    std::ostream &out, const ModelPrinter::CellInfo &cellInfo) {
  const auto &type = cellInfo.type.get();
  assert(type.isZero() || type.isOne());

  printIndent(out, 1);
  out << "assign " << ModelPrinter::PortInfo(cellInfo, 0).getName()
      << " = " << (type.isZero() ? "0" : "1") << ";\n";
}

static inline void defineInputBinding(
    std::ostream &out, const ModelPrinter::LinkInfo &linkInfo) {
  out << getLinkExpr(linkInfo);
}

static inline void defineInputBinding(
    std::ostream &out,
    const ModelPrinter::LinksInfo &linksInfo,
    size_t index,
    size_t width) {
  assert(width > 0);

  if (width == 1) {
    defineInputBinding(out, linksInfo[index]);
  } else {
    out << "{ ";
    bool comma = false;
    for (size_t i = 0; i < width; ++i) {
      // Space before "," is for escaped identifiers.
      if (comma) out << " , ";
      defineInputBinding(out, linksInfo[index + width - 1 - i]);
      comma = true;
    }
    // Space before "}" is for escaped identifiers.
    out << " }";
  }
}

static inline void defineOutputBinding(
    std::ostream &out, const ModelPrinter::PortInfo &portInfo) {
  out << portInfo.getName();
}

static inline void defineOutputBinding(
    std::ostream &out,
    const ModelPrinter::CellInfo &cellInfo,
    size_t index,
    size_t width) {
  assert(width > 0);

  if (width == 1) {
    defineOutputBinding(out, ModelPrinter::PortInfo(cellInfo, index));
  } else {
    out << "{ ";
    bool comma = false;
    for (size_t i = 0; i < width; ++i) {
      // Space before "," is for escaped identifiers.
      if (comma) out << " , ";
      const auto upperBitIndex = index + width - 1 - i;
      defineOutputBinding(out, ModelPrinter::PortInfo(cellInfo, upperBitIndex));
      comma = true;
    }
    // Space before "}" is for escaped identifiers.
    out << " }";
  }
}

static inline void instantiateCell(
    std::ostream &out,
    const ModelPrinter::CellInfo &cellInfo,
    const ModelPrinter::LinksInfo &linksInfo) {
  const auto &type = cellInfo.type.get();
  assert(!type.isIn() && !type.isOut());

  printIndent(out, 1);

  const auto instanceName = getInstanceName(cellInfo);
  out << cellInfo.getType();
  if (!instanceName.empty()) {
    out << " ";
  }
  // Space before "(" is for escaped identifiers.
  out << instanceName << " ( ";

  bool comma = false;
  if (type.isGate()) {
    assert(!type.isMaj() || linksInfo.size() == 3);

    // In built-in Verilog gates, outputs come before inputs.
    for (uint16_t output = 0; output < type.getOutNum(); ++output) {
      // Space before "," is for escaped identifiers.
      if (comma) out << " , ";
      defineOutputBinding(out, ModelPrinter::PortInfo(cellInfo, output));
      comma = true;
    }

    for (auto linkInfo : linksInfo) {
      // Space before "," is for escaped identifiers.
      if (comma) out << " , ";
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
      // Space before "," is for escaped identifiers.
      if (comma) out << " , ";
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

  // Space before ")" is for escaped identifiers.
  out << " );\n";
}

static inline void assignModelOutputs(
    std::ostream &out,
    const ModelPrinter::CellInfo &cellInfo,
    const ModelPrinter::LinksInfo &linksInfo) {
  const auto &type = cellInfo.type.get();
  assert(type.isOut());

  printIndent(out, 1);
  // Space before ";" is for escaped identifiers.
  out << "assign " << ModelPrinter::PortInfo(cellInfo, 0).getName()
      << " = " << getLinkExpr(linksInfo.front()) << " ;\n"; 
}

void VerilogPrinter::onModelBegin(std::ostream &out, const std::string &name) {
  out << "module " << name;
}

void VerilogPrinter::onModelEnd(std::ostream &out, const std::string &name) {
  out << "endmodule" << " // module " << name << "\n";
}

void VerilogPrinter::onInterfaceBegin(std::ostream &out) {
  // Space before "(" is for escaped identifiers.
  out << " (\n";
  isFirstPort = true;
}

void VerilogPrinter::onInterfaceEnd(std::ostream &out) {
  out << "\n);\n";
}

void VerilogPrinter::onType(std::ostream &out, const CellType &cellType) {
  const auto symbol = cellType.getSymbol();
  switch(symbol & ~FLGMASK) {
  case MAJ:      printMajType(out, cellType);
                 out << "\n";
                 break;
  case DFF:      printDffType(out, cellType);
                 out << "\n";
                 break;
  case sDFF:     printSDffType(out, cellType);
                 out << "\n";
                 break;
  case aDFF:     printADffType(out, cellType);
                 out << "\n";
                 break;
  case DFFrs:    printDffRsType(out, cellType);
                 out << "\n";
                 break;
  case DLATCH:   printDLatchType(out, cellType);
                 out << "\n";
                 break;
  case aDLATCH:  printADLatchType(out, cellType);
                 out << "\n";
                 break;
  case DLATCHrs: printDLatchRsType(out, cellType);
                 out << "\n";
                 break;
  case LATCHrs:  printLatchRsType(out, cellType);
                 out << "\n";
                 break;
  default:       /* print nothing */
                 break;
  }
}

void VerilogPrinter::onPort(std::ostream &out, const CellInfo &cellInfo) {
  if (!isFirstPort) {
    // Space before "," is for escaped identifiers.
    out << " ,\n";
  }

  out << "  " << (cellInfo.type.get().isIn() ? "input" : "output") << " ";
  out << PortInfo(cellInfo, 0).getName();

  isFirstPort = false;
}

void VerilogPrinter::onCell(std::ostream &out,
                            const CellInfo &cellInfo,
                            const LinksInfo &linksInfo,
                            unsigned pass) {
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

} // namespace eda::gate::model
