//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "net_printer_verilog.h"
#include "net_printer_verilog_lib.h"

#include <cassert>
#include <sstream>

namespace eda::gate::model {

static inline void printIndent(std::ostream &out, size_t n) {
  out << std::string(2 * n, ' ');
}

static inline std::string getInstanceName(
    const NetPrinter::CellInfo &cellInfo) { 
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
    const NetPrinter::LinkInfo &linkInfo) {
  std::stringstream ss;

  if (linkInfo.inv) ss << "~";
  ss << linkInfo.getSourceName();

  return ss.str();
}

static inline std::string getPinName(CellTypeID typeID, unsigned index) {
  const auto &attr = CellType::get(typeID).getAttr();
  assert(attr.hasPortInfo());

  const auto [i, j] = attr.mapPinToPort(index);
  const auto &port = attr.getPort(i);

  return port.width == 1 ? port.getName() :
      // Space before "[" is for escaped identifiers.
      fmt::format("{} [{}]", port.getName(), j);
}

static inline void declareWiresForCellOutputs(
    std::ostream &out, const NetPrinter::CellInfo &cellInfo) {
  const auto &type = cellInfo.type.get();

  if (type.isIn() || type.isOut())
    return;

  for (uint16_t output = 0; output < type.getOutNum(); ++output) {
    printIndent(out, 1);
    // Space before ";" is for escaped identifiers.
    out << "wire " << NetPrinter::PortInfo(cellInfo, output).getName() << " ;\n";
  }
}

static inline void assignConstant(
    std::ostream &out, const NetPrinter::CellInfo &cellInfo) {
  const auto &type = cellInfo.type.get();
  assert(type.isZero() || type.isOne());

  printIndent(out, 1);
  // Space before ";" is for unification.
  out << "assign " << NetPrinter::PortInfo(cellInfo, 0).getName()
      << " = " << (type.isZero() ? "0" : "1") << " ;\n";
}

static inline void defineInputBinding(
    std::ostream &out, const NetPrinter::LinkInfo &linkInfo) {
  out << getLinkExpr(linkInfo);
}

static void defineInputBinding(
    std::ostream &out,
    const NetPrinter::LinksInfo &linksInfo,
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
    std::ostream &out, const NetPrinter::PortInfo &portInfo) {
  out << portInfo.getName();
}

static void defineOutputBinding(
    std::ostream &out,
    const NetPrinter::CellInfo &cellInfo,
    size_t index,
    size_t width) {
  assert(width > 0);

  if (width == 1) {
    defineOutputBinding(out, NetPrinter::PortInfo(cellInfo, index));
  } else {
    out << "{ ";
    bool comma = false;
    for (size_t i = 0; i < width; ++i) {
      // Space before "," is for escaped identifiers.
      if (comma) out << " , ";
      const auto upperBitIndex = index + width - 1 - i;
      defineOutputBinding(out, NetPrinter::PortInfo(cellInfo, upperBitIndex));
      comma = true;
    }
    // Space before "}" is for escaped identifiers.
    out << " }";
  }
}

static void instantiateCell(
    std::ostream &out,
    const NetPrinter::CellInfo &cellInfo,
    const NetPrinter::LinksInfo &linksInfo) {
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
      defineOutputBinding(out, NetPrinter::PortInfo(cellInfo, output));
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

static inline void assignModelInput(
    std::ostream &out,
    const NetPrinter::CellInfo &cellInfo,
    const std::string &rhs) {
  printIndent(out, 1);
  // Space before ";" is for escaped identifiers.
  out << "assign " << NetPrinter::PortInfo(cellInfo, 0).getName() << " = "
                   << rhs << " ;\n";
}

static inline void assignModelOutput(
    std::ostream &out,
    const std::string &outputName,
    const NetPrinter::LinksInfo &linksInfo) {
  printIndent(out, 1);
  // Space before ";" is for escaped identifiers.
  out << "assign " << outputName << " = "
                   << getLinkExpr(linksInfo.front()) << " ;\n"; 
}

static bool defineOriginalInterface(std::ostream &out, const CellTypeID typeID) {
  if (typeID == OBJ_NULL_ID)
    return false;

  const auto &type = CellType::get(typeID);
  if (!type.hasAttr())
    return false;

  const auto &attr = type.getAttr();
  if (!attr.hasPortInfo())
    return false;

  // Space before "(" is for escaped identifiers.
  out << " (\n";
  bool comma = false;
  for (const auto &port : attr.getOrderedPorts()) {
    // Space before "," is for escaped identifiers.
    if (comma) out << " ,\n";
    printIndent(out, 1);
    out << (port.input ? "input" : "output");
    if (port.width > 1) {
      out << " [" << (port.width - 1) << ":0]";
    }
    out << " " << port.getName();
    comma = true;
  }
  out << "\n);\n";

  return true;
}

void NetPrinterVerilog::onModelBegin(std::ostream &out,
                                     const std::string &name,
                                     const CellTypeID typeID) {
  out << "module " << name;

  printOriginalInterface = defineOriginalInterface(out, typeID);
  topLevelTypeID = typeID;
}

void NetPrinterVerilog::onModelEnd(std::ostream &out,
                                   const std::string &name,
                                   const CellTypeID typeID) {
  out << "endmodule" << " // " << name << "\n";
}

void NetPrinterVerilog::onInterfaceBegin(std::ostream &out) {
  if (printOriginalInterface) return;

  // Space before "(" is for escaped identifiers.
  out << " (\n";
  pins.clear();
}

void NetPrinterVerilog::onInterfaceEnd(std::ostream &out) {
  if (printOriginalInterface) return;

  out << "\n);\n";
}

void NetPrinterVerilog::onType(std::ostream &out, const CellType &cellType) {
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

void NetPrinterVerilog::onPort(std::ostream &out,
                               const CellInfo &cellInfo,
                               unsigned index) {
  const auto &type = cellInfo.type.get();
 
  // Save mapping between input/output cells and pin indices.
  pins.emplace(cellInfo.cell, index);

  if (printOriginalInterface) {
    if (type.isIn()) {
      printIndent(out, 1);
      // Space before ";" is for escaped identifiers.
      out << "wire " << PortInfo(cellInfo, 0).getName() << " ;\n";
    }
  } else {
    if (index > 0) {
      // Space before "," is for escaped identifiers.
      out << " ,\n";
    }

    printIndent(out, 1);
    out << (type.isIn() ? "input" : "output") << " ";
    out << PortInfo(cellInfo, 0).getName();
  }
}

void NetPrinterVerilog::onCell(std::ostream &out,
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

  if (type.isIn() && printOriginalInterface) {
    const auto found = pins.find(cellInfo.cell);
    assert(found != pins.end());

    const auto rhs = getPinName(topLevelTypeID, found->second);
    assignModelInput(out, cellInfo, rhs);
    return;
  }

  if (type.isOut()) {
    std::string lhs;

    if (printOriginalInterface) {
      const auto found = pins.find(cellInfo.cell);
      assert(found != pins.end());

      lhs = getPinName(topLevelTypeID, found->second);
    } else {
      lhs = NetPrinter::PortInfo(cellInfo, 0).getName();
    }

    assignModelOutput(out, lhs, linksInfo);
    return;
  }
}

} // namespace eda::gate::model
