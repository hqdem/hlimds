//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/celltype.h"
#include "gate/model/net.h"
#include "gate/model/subnet.h"

#include <cassert>

#define CELL_TYPE(name, symbol, p0, p1, p2, p3, p4, p5, p6, p7, p8, nIn, nOut) \
  const CellTypeID CELL_TYPE_ID(symbol) = makeCellType( \
    symbol, \
    name, \
    CellProperties{p0, p1, p2, p3, p4, p5, p6, p7, p8}, \
    nIn, \
    nOut); \
  const uint32_t CELL_TYPE_SID(symbol) = CELL_TYPE_ID(symbol).getSID();

namespace eda::gate::model {

// Properties: A,B,C,D,E,F,G,H,I
//
// A = Cell
// B = Soft
// C = Combinational
// D = Constant
// E = Identity
// F = Commutative
// G = Associative
// H = Regroupable
// I = Negative

//-------------------------------------------------------------------------
//       | Name         | Symbol       | Properties        | #In   | #Out |
//       |              |              | A B C D E F G H I |       |      |
//-------------------------------------------------------------------------
CELL_TYPE("in",           IN,            1,1,0,0,0,0,0,0,0,  0,       1);
CELL_TYPE("out",          OUT,           1,1,0,0,0,0,0,0,0,  1,       0);
//-------------------------------------------------------------------------
CELL_TYPE("zero",         ZERO,          1,1,1,1,0,0,0,0,0,  0,       1);
CELL_TYPE("one",          ONE,           1,1,1,1,0,0,0,0,0,  0,       1);
//-------------------------------------------------------------------------
CELL_TYPE("buf",          BUF,           1,1,1,0,1,0,0,0,0,  1,       1);
CELL_TYPE("not",          NOT,           1,1,1,0,0,0,0,0,1,  1,       1);
CELL_TYPE("and",          AND,           1,1,1,0,0,1,1,1,0,  0xffff,  1);
CELL_TYPE("or",           OR,            1,1,1,0,0,1,1,1,0,  0xffff,  1);
CELL_TYPE("xor",          XOR,           1,1,1,0,0,1,1,1,0,  0xffff,  1);
CELL_TYPE("nand",         NAND,          1,1,1,0,0,1,0,0,1,  0xffff,  1);
CELL_TYPE("nor",          NOR,           1,1,1,0,0,1,0,0,1,  0xffff,  1);
CELL_TYPE("xnor",         XNOR,          1,1,1,0,0,1,1,0,1,  0xffff,  1);
CELL_TYPE("maj",          MAJ,           1,1,1,0,0,1,0,0,0,  0xffff,  1);
//-------------------------------------------------------------------------
CELL_TYPE("dff_p",        DFF_p,         1,1,0,0,0,0,0,0,0,  2,       1);
CELL_TYPE("dff_n",        DFF_n,         1,1,0,0,0,0,0,0,0,  2,       1);
CELL_TYPE("sdff_pp0",     sDFF_pp0,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_pp1",     sDFF_pp1,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_pn0",     sDFF_pn0,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_pn1",     sDFF_pn1,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_np0",     sDFF_np0,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_np1",     sDFF_np1,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_nn0",     sDFF_nn0,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("sdff_nn1",     sDFF_nn1,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_pp0",     aDFF_pp0,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_pp1",     aDFF_pp1,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_pn0",     aDFF_pn0,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_pn1",     aDFF_pn1,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_np0",     aDFF_np0,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_np1",     aDFF_np1,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_nn0",     aDFF_nn0,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("adff_nn1",     aDFF_nn1,      1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("dffrs_ppp",    DFFrs_ppp,     1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_ppn",    DFFrs_ppn,     1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_pnp",    DFFrs_pnp,     1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_pnn",    DFFrs_pnn,     1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_npp",    DFFrs_npp,     1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_npn",    DFFrs_npn,     1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_nnp",    DFFrs_nnp,     1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("dffrs_nnn",    DFFrs_nnn,     1,1,0,0,0,0,0,0,0,  4,       1);
//-------------------------------------------------------------------------
CELL_TYPE("latch_p",      DLATCH_p,      1,1,0,0,0,0,0,0,0,  2,       1);
CELL_TYPE("latch_n",      DLATCH_n,      1,1,0,0,0,0,0,0,0,  2,       1);
CELL_TYPE("alatch_pp0",   aDLATCH_pp0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("alatch_pp1",   aDLATCH_pp1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("alatch_pn0",   aDLATCH_pn0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("alatch_pn1",   aDLATCH_pn1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("alatch_np0",   aDLATCH_np0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("alatch_np1",   aDLATCH_np1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("alatch_nn0",   aDLATCH_nn0,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("alatch_nn1",   aDLATCH_nn1,   1,1,0,0,0,0,0,0,0,  3,       1);
CELL_TYPE("latchrs_ppp",  DLATCHrs_ppp,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("latchrs_ppn",  DLATCHrs_ppn,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("latchrs_pnp",  DLATCHrs_pnp,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("latchrs_pnn",  DLATCHrs_pnn,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("latchrs_npp",  DLATCHrs_npp,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("latchrs_npn",  DLATCHrs_npn,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("latchrs_nnp",  DLATCHrs_nnp,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("latchrs_nnn",  DLATCHrs_nnn,  1,1,0,0,0,0,0,0,0,  4,       1);
CELL_TYPE("rs_pp",        LATCHrs_pp,    1,1,0,0,0,0,0,0,0,  2,       1);
CELL_TYPE("rs_pn",        LATCHrs_pn,    1,1,0,0,0,0,0,0,0,  2,       1);
CELL_TYPE("rs_np",        LATCHrs_np,    1,1,0,0,0,0,0,0,0,  2,       1);
CELL_TYPE("rs_nn",        LATCHrs_nn,    1,1,0,0,0,0,0,0,0,  2,       1);
//-------------------------------------------------------------------------

const Net &CellType::getNet() const {
  assert(isNet());
  return Net::get(implID);
}

const Subnet &CellType::getSubnet() const {
  assert(isSubnet());
  return Subnet::get(implID);
}

//===----------------------------------------------------------------------===//
// Cell Type Validator
//===----------------------------------------------------------------------===//

#define OBJECT(type) "Type [" << type.getName() << "]"
#define PREFIX(type) OBJECT(type) << ": "

#define VALIDATE(logger, prop, msg)\
  if (!(prop)) {\
    DIAGNOSE_ERROR(logger, msg);\
    return false;\
  }

#define VALIDATE_CELLTYPE(logger, type, prop, msg)\
  VALIDATE(logger, prop, PREFIX(type) << msg)

#define VALIDATE_CELLTYPE_IN_PINS(logger, type, expected)\
  VALIDATE_CELLTYPE(logger, type,\
      type.getInNum() == (expected),\
      "Incorrect number of input pins: " << type.getInNum() <<\
          ", expected " << (expected))

#define VALIDATE_CELLTYPE_OUT_PINS(logger, type, expected)\
  VALIDATE_CELLTYPE(logger, type,\
       type.getOutNum() == (expected),\
      "Incorrect number of output pins: " << type.getOutNum() <<\
          ", expected " << (expected))

#define VALIDATE_CELLTYPE_IN_PINS_ge(logger, type, bound)\
  VALIDATE_CELLTYPE(logger, type,\
      !type.isInNumFixed() || type.getInNum() >= (bound),\
      "Incorrect number of input pins: " << type.getInNum() <<\
          ", expected >= " << (bound))

#define VALIDATE_CELLTYPE_IN_PORTS(logger, type, expected)\
  VALIDATE_CELLTYPE(logger, type,\
      type.getAttr().getInPortNum() == (expected),\
      "Incorrect number of input ports: " << type.getAttr().getInPortNum() <<\
          ", expected " << (expected))

#define VALIDATE_CELLTYPE_OUT_PORTS(logger, type, expected)\
  VALIDATE_CELLTYPE(logger, type,\
       type.getAttr().getOutPortNum() == (expected),\
      "Incorrect number of output ports: " << type.getAttr().getOutPortNum() <<\
          ", expected " << (expected))

#define VALIDATE_CELLTYPE_IN_WIDTH(logger, type, port, width)\
  VALIDATE_CELLTYPE(logger, type,\
       type.getAttr().getInWidth(port) == (width),\
      "Incorrect width of input port #" << port << ": " <<\
          type.getAttr().getInWidth(port) << ", expected " << (width))

#define VALIDATE_CELLTYPE_OUT_WIDTH(logger, type, port, width)\
  VALIDATE_CELLTYPE(logger, type,\
       type.getAttr().getOutWidth(port) == (width),\
      "Incorrect width of output port #" << port << ": " <<\
          type.getAttr().getOutWidth(port) << ", expected " << (width))

#define VALIDATE_CELLTYPE_IN_IN_WIDTHS(logger, type, i, j)\
  VALIDATE_CELLTYPE(logger, type,\
       type.getAttr().getInWidth(i) == type.getAttr().getInWidth(j),\
      "Input ports #" << i << " and #" << j <<\
          " have different widths")

#define VALIDATE_CELLTYPE_OUT_OUT_WIDTHS(logger, type, i, j)\
  VALIDATE_CELLTYPE(logger, type,\
       type.getAttr().getOutWidth(i) == type.getAttr().getOutWidth(j),\
      "Output ports #" << i << " and #" << j <<\
          " have different widths")

#define VALIDATE_CELLTYPE_IN_OUT_WIDTHS(logger, type, i, j)\
  VALIDATE_CELLTYPE(logger, type,\
       type.getAttr().getInWidth(i) == type.getAttr().getOutWidth(j),\
      "Input ports #" << i << " and output port #" << j <<\
          " have different widths")

/// Validates IN.
static bool validateIn(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 0);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates OUT.
static bool validateOut(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 0);
  return true;
}

/// Validates ZERO and ONE.
static bool validateConst(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 0);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates BUF and NOT.
static bool validateLogic1(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates AND, OR, XOR, NAND, NOR, and XNOR.
static bool validateLogic2plus(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PINS_ge(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates MAJ.
static bool validateLogicMaj(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PINS_ge(logger, type, 3);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates DFF*.
static bool validateDff(const CellType &type, diag::Logger &logger) {
  // D flip-flop (Q, D, CLK):
  // Q(t) = CLK(posedge) ? D : Q(t-1).
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates sDFF*.
static bool validateSDff(const CellType &type, diag::Logger &logger) {
  // D flip-flop w/ synchronous reset (Q, D, CLK, RST):
  // Q(t) = CLK(posedge) ? (RST ? 0 : D) : Q(t-1).
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 3);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates aDFF*.
static bool validateADff(const CellType &type, diag::Logger &logger) {
  // D flip-flop w/ asynchronous reset (Q, D, CLK, RST):
  // Q(t) = RST(level=1) ? 0 : (CLK(posedge) ? D : Q(t-1)).
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 3);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates DFFrs*.
static bool validateDffRs(const CellType &type, diag::Logger &logger) {
  // D flip-flop w/ (asynchronous) reset and set (Q, D, CLK, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : (CLK(posedge) ? D : Q(t-1))).
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 4);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates DLATCH*.
static bool validateDLatch(const CellType &type, diag::Logger &logger) {
  // D latch (Q, D, ENA):
  // Q(t) = ENA(level=1) ? D : Q(t-1).
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates aDLATCH*.
static bool validateADLatch(const CellType &type, diag::Logger &logger) {
  // D latch w/ asynchronous reset (Q, D, ENA, RST):
  // Q(t) = RST(level=1) ? 0 : (ENA(level=1) ? D : Q(t-1)).
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 3);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates DLATCHrs*.
static bool validateDLatchRs(const CellType &type, diag::Logger &logger) {
  // D latch w/ (asynchronous) reset and set (Q, D, ENA, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : (ENA(level=1) ? D : Q(t-1))).
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 4);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates LATCHrs*.
static bool validateLatchRs(const CellType &type, diag::Logger &logger) {
  // RS latch (Q, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : Q(t-1)).
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return true;
}

/// Validates BNOT.
static bool validateBitwise1(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_IN_OUT_WIDTHS(logger, type, 0, 0);
  return true;
}

/// Validates BAND, BOR, BXOR, BNAND, BNOR, and BXNOR.
static bool validateBitwise2(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_IN_IN_WIDTHS(logger, type, 0, 1);
  VALIDATE_CELLTYPE_IN_OUT_WIDTHS(logger, type, 0, 0);
  return true;
}

/// Validates RAND, ROR, RXOR, RNAND, RNOR, and RXNOR.
static bool validateReduce(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_WIDTH(logger, type, 0, 1);
  return true;
}

/// Validates MUX2.
static bool validateMux2(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 3);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_IN_WIDTH(logger, type, 0, 1);
  VALIDATE_CELLTYPE_IN_IN_WIDTHS(logger, type, 1, 2);
  VALIDATE_CELLTYPE_IN_OUT_WIDTHS(logger, type, 1, 0);
  return true;
}

/// Validates SHL and SHR*.
static bool validateShift(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  return true;
}

/// Validates EQ*, NEQ*, EQX*, NEQX*, LT*, LTE*, GT*, and GTE*.
static bool validateCompare(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_WIDTH(logger, type, 0, 1);
  return true;
}

/// Validates NEG.
static bool validateArith1(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  return true;
}

/// Validates ADD, SUB, MUL*, DIV*, REM*, and MOD*.
static bool validateArith2(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  return true;
}

/// Validates UNDEF.
static bool validateUndef(const CellType &type, diag::Logger &logger) {
  const auto &attr = type.getAttr();
  const auto ports = attr.getOrderedPorts();

  size_t nIn{0}, nOut{0}, wIn{0}, wOut{0};
  for (const auto &port : ports) {
    VALIDATE_CELLTYPE(logger, type, port.width > 0, "Zero port width");
    if (port.input) {
      nIn += 1;
      wIn += port.width;
    } else {
      nOut += 1;
      wOut += port.width;
    }
  }

  VALIDATE_CELLTYPE_IN_PORTS(logger, type, nIn);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, nOut);
  VALIDATE_CELLTYPE_IN_PINS(logger, type, wIn);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, wOut);
  VALIDATE_CELLTYPE(logger, type,
      (nIn + nOut) <= CellTypeAttr::MaxPortNum,
      "Too many input/output ports");
  VALIDATE_CELLTYPE(logger, type,
      (wIn + wOut) <= CellTypeAttr::MaxBitWidth,
      "Too many input/output pins");

  return true;
}

bool validateCellType(const CellType &type, diag::Logger &logger) {
  VALIDATE_CELLTYPE(logger, type,
      type.isGate() || type.hasAttr(),
      "Non-gate cell has no attributes");

  if (type.isNet()) {
    const auto &net = type.getNet();
    VALIDATE_CELLTYPE(logger, type,
        net.getInNum() == type.getInNum(),
        "Incorrect number of input pins in the net implementation");
    VALIDATE_CELLTYPE(logger, type,
        net.getOutNum() == type.getOutNum(),
        "Incorrect number of output pints in the net implementation");
    VALIDATE_CELLTYPE(logger, type, validateNet(net, logger),
        "[Invalid net implementation]");
  } else if (type.isSubnet()) {
    const auto &subnet = type.getSubnet();
    VALIDATE_CELLTYPE(logger, type,
        subnet.getInNum() == type.getInNum(),
        "Incorrect number of input pins in the subnet implementation");
    VALIDATE_CELLTYPE(logger, type,
        subnet.getOutNum() == type.getOutNum(),
        "Incorrect number of output pins in the subnet implementation");
    VALIDATE_CELLTYPE(logger, type, validateSubnet(subnet, logger),
        "[Invalid subnet implementation]");
  }

  switch(type.getSymbol() & ~FLGMASK) {
  case IN:       return validateIn(type, logger);
  case OUT:      return validateOut(type, logger);
  case ZERO:     // Constants
  case ONE:      return validateConst(type, logger);
  case BUF:      // Unary logic gates
  case NOT:      return validateLogic1(type, logger);
  case AND:      // Binary logic gates
  case OR:       //
  case XOR:      //
  case NAND:     //
  case NOR:      //
  case XNOR:     return validateLogic2plus(type, logger);
  case MAJ:      return validateLogicMaj(type, logger);
  case DFF:      return validateDff(type, logger);
  case sDFF:     return validateSDff(type, logger);
  case aDFF:     return validateADff(type, logger);
  case DFFrs:    return validateDffRs(type, logger);
  case DLATCH:   return validateDLatch(type, logger);
  case aDLATCH:  return validateADLatch(type, logger);
  case DLATCHrs: return validateDLatchRs(type, logger);
  case LATCHrs:  return validateLatchRs(type, logger);
  case BNOT:     return validateBitwise1(type, logger);
  case BAND:     // Binary bitwise operations
  case BOR:      //
  case BXOR:     //
  case BNAND:    //
  case BNOR:     //
  case BXNOR:    return validateBitwise2(type, logger);
  case RAND:     // Reduction operations
  case ROR:      //
  case RXOR:     //
  case RNAND:    //
  case RNOR:     //
  case RXNOR:    return validateReduce(type, logger);
  case MUX2:     return validateMux2(type, logger);
  case SHL:      // Shift operations
  case SHRs:     //
  case SHRu:     return validateShift(type, logger);
  case EQs:      // Comparison operations
  case EQu:      //
  case NEQs:     //
  case NEQu:     //
  case EQXs:     //
  case EQXu:     //
  case NEQXs:    //
  case NEQXu:    //
  case LTs:      //
  case LTu:      //
  case LTEs:     //
  case LTEu:     //
  case GTs:      //
  case GTu:      //
  case GTEs:     //
  case GTEu:     return validateCompare(type, logger);
  case NEG:      return validateArith1(type, logger);
  case ADD:      // Binary arithmetic operations
  case SUB:      //
  case MULs:     //
  case MULu:     //
  case DIVs:     //
  case DIVu:     //
  case REMs:     //
  case REMu:     //
  case MODs:     return validateArith2(type, logger);
  case UNDEF:    return validateUndef(type, logger);
  }

  DIAGNOSE_ERROR(logger,
      PREFIX(type) << "Unknown cell type symbol: " << type.getSymbol());

  return false;
}

} // namespace eda::gate::model
