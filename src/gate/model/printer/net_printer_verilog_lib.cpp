//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "net_printer_verilog_lib.h"

namespace eda::gate::model {

void printVerilogMajType(std::ostream &out, const CellType &type) {
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

static inline std::string getEdge(const bool positive) {
  return positive ? "posedge" : "negedge";
}

static inline std::string getLevel(const bool positive) {
  return positive ? "" : "~";
}

static inline std::string getValue(const bool positive) {
  return positive ? "1" : "0";
}

static inline std::string getClkEdge(const CellType &type) {
  return getEdge(type.getClkEdge());
}

static inline std::string getRstEdge(const CellType &type) {
  return getEdge(type.getRstLevel());
}

static inline std::string getSetEdge(const CellType &type) {
  return getEdge(type.getSetLevel());
}

static inline std::string getEnaLevel(const CellType &type) {
  return getLevel(type.getEnaLevel());
}

static inline std::string getRstLevel(const CellType &type) {
  return getLevel(type.getRstLevel());
}

static inline std::string getSetLevel(const CellType &type) {
  return getLevel(type.getSetLevel());
}

static inline std::string getRstValue(const CellType &type) {
  return getValue(type.getRstValue());
}

void printVerilogDffType(std::ostream &out, const CellType &type) {
  // D flip-flop (Q, D, CLK):
  // Q(t) = CLK(posedge) ? D : Q(t-1).
  out << "module " << type.getName()
      << "(output reg Q, input D, input CLK);\n";
  out << "  always @(" << getClkEdge(type) << " CLK) begin\n";
  out << "    Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printVerilogSDffType(std::ostream &out, const CellType &type) {
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

void printVerilogADffType(std::ostream &out, const CellType &type) {
  // D flip-flop w/ asynchronous reset (Q, D, CLK, RST):
  // Q(t) = RST(level=1) ? 0 : (CLK(posedge) ? D : Q(t-1)).
  out << "module " << type.getName()
      << "(output reg Q, input D, input CLK, input RST);\n";
  out << "  always @(" << getClkEdge(type) << " CLK or "
                       << getRstEdge(type) << " RST) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= " << getRstValue(type) << ";\n";
  out << "    else\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printVerilogDffRsType(std::ostream &out, const CellType &type) {
  // D flip-flop w/ (asynchronous) reset and set (Q, D, CLK, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : (CLK(posedge) ? D : Q(t-1))).
  out << "module " << type.getName()
      << "(output reg Q, input D, input CLK, input RST, input SET);\n";
  out << "  always @(" << getClkEdge(type) << " CLK or "
                       << getRstEdge(type) << " RST or "
                       << getSetEdge(type) << " SET) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= 0;\n";
  out << "    else if (" << getSetLevel(type) << "SET)\n";
  out << "      Q <= 1;\n";
  out << "    else\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printVerilogDLatchType(std::ostream &out, const CellType &type) {
  // D latch (Q, D, ENA):
  // Q(t) = ENA(level=1) ? D : Q(t-1).
  out << "module " << type.getName()
      << "(output reg Q, input D, input ENA);\n";
  out << "  always @(*) begin\n";
  out << "    if (" << getEnaLevel(type) << "ENA)\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printVerilogADLatchType(std::ostream &out, const CellType &type) {
  // D latch w/ asynchronous reset (Q, D, ENA, RST):
  // Q(t) = RST(level=1) ? 0 : (ENA(level=1) ? D : Q(t-1)).
  out << "module " << type.getName()
      << "(output reg Q, input D, input ENA, input RST);\n";
  out << "  always @(*) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= " << getRstValue(type) << ";\n";
  out << "    else if(" << getEnaLevel(type) << "ENA)\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printVerilogDLatchRsType(std::ostream &out, const CellType &type) {
  // D latch w/ (asynchronous) reset and set (Q, D, ENA, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : (ENA(level=1) ? D : Q(t-1))).
  out << "module " << type.getName()
      << "(output reg Q, input D, input ENA, input RST, input SET);\n";
  out << "  always @(*) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= 0;\n";
  out << "    else if(" << getSetLevel(type) << "SET)\n";
  out << "      Q <= 1;\n";
  out << "    else if(" << getEnaLevel(type) << "ENA)\n";
  out << "      Q <= D;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

void printVerilogLatchRsType(std::ostream &out, const CellType &type) {
  // RS latch (Q, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : Q(t-1)).
  out << "module " << type.getName()
      << "(output reg Q, input RST, input SET);\n";
  out << "  always @(*) begin\n";
  out << "    if (" << getRstLevel(type) << "RST)\n";
  out << "      Q <= 0;\n";
  out << "    else if (" << getSetLevel(type) << "SET)\n";
  out << "      Q <= 1;\n";
  out << "  end\n";
  out << "endmodule // " << type.getName() << "\n";
}

static void printVerilogBuiltInType(std::ostream &out, const CellType &type) {
  // Built-in Verilog gate are not printed.
  out << "// Built-in Verilog construct: " << type.getName() << "\n";
}

void printVerilogCellType(std::ostream &out, const CellType &type) {
  if (type.isMaj()) {
    printVerilogMajType(out, type);
  } else if (type.isDff()) {
    printVerilogDffType(out, type);
  } else if (type.isSDff()) {
    printVerilogSDffType(out, type);
  } else if (type.isADff()) {
    printVerilogADffType(out, type);
  } else if (type.isDffRs()) {
    printVerilogDffRsType(out, type);
  } else if (type.isDLatch()) {
    printVerilogDLatchType(out, type);
  } else if (type.isADLatch()) {
    printVerilogADLatchType(out, type);
  } else if (type.isDLatchRs()) {
    printVerilogDLatchRsType(out, type);
  } else if (type.isLatchRs()) {
    printVerilogLatchRsType(out, type);
  } else {
    printVerilogBuiltInType(out, type);
  }
}

void printVerilogLibrary(std::ostream &out) {
  out << "//===" << std::string(70, '-') << "===//\n";
  out << "//\n";
  out << "// Part of the Utopia EDA Project, under the Apache License v2.0\n";
  out << "// SPDX-License-Identifier: Apache-2.0\n";
  out << "// Copyright 2024 ISP RAS (http://www.ispras.ru)\n";
  out << "//\n";
  out << "//===" << std::string(70, '-') << "===//\n";

  #define UTOPIA_FOREACH_GATE(S)\
      out << "\n";\
      printVerilogCellType(out, CellType::get(CELL_TYPE_ID(S)));
  #include "gate/model/celltype_gates.inc"
}

} // namespace eda::gate::model
