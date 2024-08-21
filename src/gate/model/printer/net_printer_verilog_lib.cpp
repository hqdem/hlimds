//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "net_printer_verilog_lib.h"

namespace eda::gate::model {

void printMajType(std::ostream &out, const CellType &type) {
  static constexpr enum { MODULE, UDP } MajMethod = MODULE;

  if constexpr (MajMethod == UDP) {
    out << "primitive " << type.getName()
        << "(output OUT, input X, input Y, input Z);\n";
    out << "  table\n";
    out << "    // X Y Z   OUT\n";
    out << "       0 0 0 : 0;\n";
    out << "       0 0 1 : 0;\n";
    out << "       0 1 0 : 0;\n";
    out << "       0 1 1 : 1;\n";
    out << "       1 0 0 : 0;\n";
    out << "       1 0 1 : 1;\n";
    out << "       1 1 0 : 1;\n";
    out << "       1 1 1 : 1;\n";
    out << "  endtable\n";
    out << "endprimitive // " << type.getName() << "\n";
  } else {
    out << "module " << type.getName()
        << "(output OUT, input X, input Y, input Z);\n";
    out << "  assign OUT = (X & Y) | (X & Z) | (Y & Z);\n";
    out << "endmodule // " << type.getName() << "\n";
  }
}

static inline std::string getClkEdge(const CellType &type) {
  return type.getClkEdge() ? "posedge" : "negedge";
}

static inline std::string getEnaLevel(const CellType &type) {
  return type.getEnaLevel() ? "" : "~";
} 

static inline std::string getRstLevel(const CellType &type) {
  return type.getRstLevel() ? "" : "~";
}

static inline std::string getSetLevel(const CellType &type) {
  return type.getSetLevel() ? "" : "~";
}

static inline std::string getRstValue(const CellType &type) {
  return type.getRstValue() ? "1" : "0";
}

void printDffType(std::ostream &out, const CellType &type) {
  // D flip-flop (Q, D, CLK):
  // Q(t) = CLK(posedge) ? D : Q(t-1).
  out << "module " << type.getName()
      << "(output reg Q, input D, input CLK);\n";
  out << "  always @(" << getClkEdge(type) << " CLK) begin\n";
  out << "    Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printSDffType(std::ostream &out, const CellType &type) {
  // D flip-flop w/ synchronous reset (Q, D, CLK, RST):
  // Q(t) = CLK(posedge) ? (RST ? 0 : D) : Q(t-1).
  out << "module " << type.getName()
      << "(output reg Q, input D, input CLK, input RST);\n";
  out << "  always @(" << getClkEdge(type) << " CLK) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= " << getRstValue(type) << ";\n";
  out << "    else\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printADffType(std::ostream &out, const CellType &type) {
  // D flip-flop w/ asynchronous reset (Q, D, CLK, RST):
  // Q(t) = RST(level=1) ? 0 : (CLK(posedge) ? D : Q(t-1)).
  out << "module " << type.getName()
      << "(output reg Q, input D, input CLK, input RST);\n";
  out << "  always @(" << getClkEdge(type) << " CLK or RST) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= " << getRstValue(type) << ";\n";
  out << "    else\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printDffRsType(std::ostream &out, const CellType &type) {
  // D flip-flop w/ (asynchronous) reset and set (Q, D, CLK, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : (CLK(posedge) ? D : Q(t-1))).
  out << "module " << type.getName()
      << "(output reg Q, input D, input CLK, input RST, input SET);\n";
  out << "  always @(" << getClkEdge(type) << " CLK or RST or SET) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= 0;\n";
  out << "    else if (" << getSetLevel(type) << "set)\n";
  out << "      Q <= 1;\n";
  out << "    else\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printDLatchType(std::ostream &out, const CellType &type) {
  // D latch (Q, D, ENA):
  // Q(t) = ENA(level=1) ? D : Q(t-1).
  out << "module " << type.getName()
      << "(output reg Q, input D, input ENA);\n";
  out << "  always @(ENA) begin\n";
  out << "    if (" << getEnaLevel(type) << "ENA)\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printADLatchType(std::ostream &out, const CellType &type) {
  // D latch w/ asynchronous reset (Q, D, ENA, RST):
  // Q(t) = RST(level=1) ? 0 : (ENA(level=1) ? D : Q(t-1)).
  out << "module " << type.getName()
      << "(output reg Q, input D, input ENA, input RST);\n";
  out << "  always @(ENA or RST) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= " << getRstValue(type) << ";\n";
  out << "    else if(" << getEnaLevel(type) << "ENA)\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printDLatchRsType(std::ostream &out, const CellType &type) {
  // D latch w/ (asynchronous) reset and set (Q, D, ENA, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : (ENA(level=1) ? D : Q(t-1))).
  out << "module " << type.getName()
      << "(output reg Q, input D, input ENA, input RST, input SET);\n";
  out << "  always @(ENA or RST or SET) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= 0;\n";
  out << "    else if(" << getSetLevel(type) << "SET)\n";
  out << "      Q <= 1;\n";
  out << "    else if(" << getEnaLevel(type) << "ENA)\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printLatchRsType(std::ostream &out, const CellType &type) {
  // RS latch (Q, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : Q(t-1)).
  out << "module " << type.getName()
      << "(output reg Q, input RST, input SET);\n";
  out << "  always @(RST or SET) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= 0;\n";
  out << "    else if (" << getSetLevel(type) << "SET)\n";
  out << "      Q <= 1;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

} // namespace eda::gate::model
