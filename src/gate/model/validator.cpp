//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/validator.h"

#include <fmt/format.h>

#define VALIDATE(logger, prop, msg)\
  if (!(prop)) {\
    UTOPIA_RAISE_ERROR(logger, msg);\
    passed = false;\
  }

#define VALIDATE_QUIET(prop)\
  passed &= (prop)

#define VALIDATE_GROUP_BEGIN(logger, msg)\
  UTOPIA_RAISE_BEGIN(logger, msg)

#define VALIDATE_GROUP_END(logger)\
  UTOPIA_RAISE_END(logger)

#define VALIDATE_CELLTYPE_IN_PINS(logger, type, expected)\
  VALIDATE(logger, type.getInNum() == (expected),\
      "Incorrect number of input pins: " << type.getInNum() <<\
      ", expected " << (expected))

#define VALIDATE_CELLTYPE_OUT_PINS(logger, type, expected)\
  VALIDATE(logger, type.getOutNum() == (expected),\
      "Incorrect number of output pins: " << type.getOutNum() <<\
      ", expected " << (expected))

#define VALIDATE_CELLTYPE_IN_PINS_ge(logger, type, bound)\
  VALIDATE(logger, !type.isInNumFixed() || type.getInNum() >= (bound),\
      "Incorrect number of input pins: " << type.getInNum() <<\
      ", expected >= " << (bound))

#define VALIDATE_CELLTYPE_IN_PORTS(logger, type, expected)\
  VALIDATE(logger, type.getAttr().getInPortNum() == (expected),\
      "Incorrect number of input ports: " << type.getAttr().getInPortNum() <<\
      ", expected " << (expected))

#define VALIDATE_CELLTYPE_OUT_PORTS(logger, type, expected)\
  VALIDATE(logger, type.getAttr().getOutPortNum() == (expected),\
      "Incorrect number of output ports: " << type.getAttr().getOutPortNum() <<\
      ", expected " << (expected))

#define VALIDATE_CELLTYPE_IN_WIDTH(logger, type, port, width)\
  VALIDATE(logger, type.getAttr().getInWidth(port) == (width),\
      "Incorrect width of input port #" << port << ": " <<\
       type.getAttr().getInWidth(port) << ", expected " << (width))

#define VALIDATE_CELLTYPE_OUT_WIDTH(logger, type, port, width)\
  VALIDATE(logger, type.getAttr().getOutWidth(port) == (width),\
      "Incorrect width of output port #" << port << ": " <<\
       type.getAttr().getOutWidth(port) << ", expected " << (width))

#define VALIDATE_CELLTYPE_IN_IN_WIDTHS(logger, type, i, j)\
  VALIDATE(logger, type.getAttr().getInWidth(i) == type.getAttr().getInWidth(j),\
      "Input ports #" << i << " and #" << j <<\
      " have different widths")

#define VALIDATE_CELLTYPE_OUT_OUT_WIDTHS(logger, type, i, j)\
  VALIDATE(logger, type.getAttr().getOutWidth(i) == type.getAttr().getOutWidth(j),\
      "Output ports #" << i << " and #" << j <<\
      " have different widths")

#define VALIDATE_CELLTYPE_IN_OUT_WIDTHS(logger, type, i, j)\
  VALIDATE(logger, type.getAttr().getInWidth(i) == type.getAttr().getOutWidth(j),\
      "Input ports #" << i << " and output port #" << j <<\
      " have different widths")

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Cell Type Validator
//===----------------------------------------------------------------------===//

static inline std::string debugInfo(const CellType &type) {
  return fmt::format("cell-type '{}'", type.getName());
}

/// Validates IN.
static bool validateIn(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 0);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates OUT.
static bool validateOut(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 0);
  return passed;
}

/// Validates ZERO and ONE.
static bool validateConst(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 0);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates BUF and NOT.
static bool validateLogic1(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates AND, OR, XOR, NAND, NOR, and XNOR.
static bool validateLogic2plus(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS_ge(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates MAJ.
static bool validateLogicMaj(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS_ge(logger, type, 3);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates DFF*.
static bool validateDff(const CellType &type, diag::Logger &logger) {
  // D flip-flop (Q, D, CLK):
  // Q(t) = CLK(posedge) ? D : Q(t-1).
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates sDFF*.
static bool validateSDff(const CellType &type, diag::Logger &logger) {
  // D flip-flop w/ synchronous reset (Q, D, CLK, RST):
  // Q(t) = CLK(posedge) ? (RST ? 0 : D) : Q(t-1).
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 3);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates aDFF*.
static bool validateADff(const CellType &type, diag::Logger &logger) {
  // D flip-flop w/ asynchronous reset (Q, D, CLK, RST):
  // Q(t) = RST(level=1) ? 0 : (CLK(posedge) ? D : Q(t-1)).
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 3);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates DFFrs*.
static bool validateDffRs(const CellType &type, diag::Logger &logger) {
  // D flip-flop w/ (asynchronous) reset and set (Q, D, CLK, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : (CLK(posedge) ? D : Q(t-1))).
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 4);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates DLATCH*.
static bool validateDLatch(const CellType &type, diag::Logger &logger) {
  // D latch (Q, D, ENA):
  // Q(t) = ENA(level=1) ? D : Q(t-1).
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates aDLATCH*.
static bool validateADLatch(const CellType &type, diag::Logger &logger) {
  // D latch w/ asynchronous reset (Q, D, ENA, RST):
  // Q(t) = RST(level=1) ? 0 : (ENA(level=1) ? D : Q(t-1)).
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 3);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates DLATCHrs*.
static bool validateDLatchRs(const CellType &type, diag::Logger &logger) {
  // D latch w/ (asynchronous) reset and set (Q, D, ENA, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : (ENA(level=1) ? D : Q(t-1))).
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 4);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates LATCHrs*.
static bool validateLatchRs(const CellType &type, diag::Logger &logger) {
  // RS latch (Q, RST, SET):
  // Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : Q(t-1)).
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PINS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PINS(logger, type, 1);
  return passed;
}

/// Validates BNOT.
static bool validateBitwise1(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  // Extension is allowed.
#if 0
  VALIDATE_CELLTYPE_IN_OUT_WIDTHS(logger, type, 0, 0);
#endif
  return passed;
}

/// Validates BAND, BOR, BXOR, BNAND, BNOR, and BXNOR.
static bool validateBitwise2(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  // Extension is allowed.
#if 0
  VALIDATE_CELLTYPE_IN_IN_WIDTHS(logger, type, 0, 1);
  VALIDATE_CELLTYPE_IN_OUT_WIDTHS(logger, type, 0, 0);
#endif
  return passed;
}

/// Validates RAND, ROR, RXOR, RNAND, RNOR, and RXNOR.
static bool validateReduce(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  // Extension is allowed.
#if 0
  VALIDATE_CELLTYPE_OUT_WIDTH(logger, type, 0, 1);
#endif
  return passed;
}

/// Validates MUX2.
static bool validateMux2(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 3);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_IN_WIDTH(logger, type, 0, 1);
  VALIDATE_CELLTYPE_IN_IN_WIDTHS(logger, type, 1, 2);
  VALIDATE_CELLTYPE_IN_OUT_WIDTHS(logger, type, 1, 0);
  return passed;
}

/// Validates SHL and SHR*.
static bool validateShift(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  return passed;
}

/// Validates EQ*, NEQ*, EQX*, NEQX*, LT*, LTE*, GT*, and GTE*.
static bool validateCompare(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_WIDTH(logger, type, 0, 1);
  return passed;
}

/// Validates NEG.
static bool validateArith1(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 1);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  return passed;
}

/// Validates ADD, SUB, MUL*, DIV*, REM*, and MOD*.
static bool validateArith2(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_CELLTYPE_IN_PORTS(logger, type, 2);
  VALIDATE_CELLTYPE_OUT_PORTS(logger, type, 1);
  return passed;
}

/// Validates UNDEF.
static bool validateUndef(const CellType &type, diag::Logger &logger) {
  bool passed = true;

  const auto &attr = type.getAttr();
  const auto ports = attr.getOrderedPorts();

  size_t nIn{0}, nOut{0}, wIn{0}, wOut{0};
  for (const auto &port : ports) {
    VALIDATE(logger, port.width > 0, "Zero port width");
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
  VALIDATE(logger, ((nIn + nOut) <= CellTypeAttr::MaxPortNum),
      "Too many input/output ports");
  VALIDATE(logger, ((wIn + wOut) <= CellTypeAttr::MaxBitWidth),
      "Too many input/output pins");

  return passed;
}

bool validateCellType(const CellType &type, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_GROUP_BEGIN(logger, debugInfo(type));

  VALIDATE(logger, (type.isGate() || type.hasAttr()),
      "Non-gate cell has no attributes");

  if (type.isNet()) {
    const auto &net = type.getNet();
    VALIDATE(logger, (net.getInNum() == type.getInNum()),
        "Incorrect number of input pins in the net implementation");
    VALIDATE(logger, (net.getOutNum() == type.getOutNum()),
        "Incorrect number of output pints in the net implementation");
    VALIDATE_QUIET(validateNet(net, logger));
  } else if (type.isSubnet()) {
    const auto &subnet = type.getSubnet();
    VALIDATE(logger, (subnet.getInNum() == type.getInNum()),
        "Incorrect number of input pins in the subnet implementation");
    VALIDATE(logger, (subnet.getOutNum() == type.getOutNum()),
        "Incorrect number of output pins in the subnet implementation");
    VALIDATE_QUIET(validateSubnet(subnet, logger));
  }

  switch(type.getSymbol() & ~FLGMASK) {
  case IN:       VALIDATE_QUIET(validateIn(type, logger));
                 break;
  case OUT:      VALIDATE_QUIET(validateOut(type, logger));
                 break;
  case ZERO:     [[fallthrough]]; // Constants
  case ONE:      VALIDATE_QUIET(validateConst(type, logger));
                 break;
  case BUF:      [[fallthrough]]; // Unary logic gates
  case NOT:      VALIDATE_QUIET(validateLogic1(type, logger));
                 break;
  case AND:      [[fallthrough]]; // Binary logic gates
  case OR:       [[fallthrough]];
  case XOR:      [[fallthrough]];
  case NAND:     [[fallthrough]];
  case NOR:      [[fallthrough]];
  case XNOR:     VALIDATE_QUIET(validateLogic2plus(type, logger));
                 break;
  case MAJ:      VALIDATE_QUIET(validateLogicMaj(type, logger));
                 break;
  case DFF:      VALIDATE_QUIET(validateDff(type, logger));
                 break;
  case sDFF:     VALIDATE_QUIET(validateSDff(type, logger));
                 break;
  case aDFF:     VALIDATE_QUIET(validateADff(type, logger));
                 break;
  case DFFrs:    VALIDATE_QUIET(validateDffRs(type, logger));
                 break;
  case DLATCH:   VALIDATE_QUIET(validateDLatch(type, logger));
                 break;
  case aDLATCH:  VALIDATE_QUIET(validateADLatch(type, logger));
                 break;
  case DLATCHrs: VALIDATE_QUIET(validateDLatchRs(type, logger));
                 break;
  case LATCHrs:  VALIDATE_QUIET(validateLatchRs(type, logger));
                 break;
  case BNOT:     VALIDATE_QUIET(validateBitwise1(type, logger));
                 break;
  case BAND:     [[fallthrough]]; // Binary bitwise operations
  case BOR:      [[fallthrough]];
  case BXOR:     [[fallthrough]];
  case BNAND:    [[fallthrough]];
  case BNOR:     [[fallthrough]];
  case BXNOR:    VALIDATE_QUIET(validateBitwise2(type, logger));
                 break;
  case RAND:     [[fallthrough]]; // Reduction operations
  case ROR:      [[fallthrough]];
  case RXOR:     [[fallthrough]];
  case RNAND:    [[fallthrough]];
  case RNOR:     [[fallthrough]];
  case RXNOR:    VALIDATE_QUIET(validateReduce(type, logger));
                 break;
  case MUX2:     VALIDATE_QUIET(validateMux2(type, logger));
                 break;
  case SHLs:     [[fallthrough]]; // Shift operations
  case SHLu:     [[fallthrough]];
  case SHRs:     [[fallthrough]];
  case SHRu:     VALIDATE_QUIET(validateShift(type, logger));
                 break;
  case EQs:      [[fallthrough]]; // Comparison operations
  case EQu:      [[fallthrough]];
  case NEQs:     [[fallthrough]];
  case NEQu:     [[fallthrough]];
  case EQXs:     [[fallthrough]];
  case EQXu:     [[fallthrough]];
  case NEQXs:    [[fallthrough]];
  case NEQXu:    [[fallthrough]];
  case LTs:      [[fallthrough]];
  case LTu:      [[fallthrough]];
  case LTEs:     [[fallthrough]];
  case LTEu:     [[fallthrough]];
  case GTs:      [[fallthrough]];
  case GTu:      [[fallthrough]];
  case GTEs:     [[fallthrough]];
  case GTEu:     VALIDATE_QUIET(validateCompare(type, logger));
                 break;
  case NEG:      VALIDATE_QUIET(validateArith1(type, logger));
                 break;
  case ADDs:     [[fallthrough]]; // Binary arithmetic operations
  case ADDu:     [[fallthrough]];
  case SUBs:     [[fallthrough]];
  case SUBu:     [[fallthrough]];
  case MULs:     [[fallthrough]];
  case MULu:     [[fallthrough]];
  case DIVs:     [[fallthrough]];
  case DIVu:     [[fallthrough]];
  case REMs:     [[fallthrough]];
  case REMu:     [[fallthrough]];
  case MODs:     VALIDATE_QUIET(validateArith2(type, logger));
                 break;
  case UNDEF:    VALIDATE_QUIET(validateUndef(type, logger));
                 break;
  default:       VALIDATE(logger, false, "Unknown cell symbol");
                 break;
  }

  VALIDATE_GROUP_END(logger);
  return passed;
}

bool validateCellType(const CellTypeID typeID, diag::Logger &logger) {
  bool passed = true;
  VALIDATE(logger, (typeID != OBJ_NULL_ID), "Null cell-type identifier");
  return passed && validateCellType(CellType::get(typeID), logger);
}

//===----------------------------------------------------------------------===//
// Net Validator
//===----------------------------------------------------------------------===//

static inline std::string debugInfo(const CellID &cellID) {
  const auto &cell = Cell::get(cellID);
  const auto &type = cell.getType();
  return fmt::format("cell#{}:{}", cellID.getSID(), type.getName());
}

static inline std::string debugInfo(const LinkEnd &linkEnd, const size_t i) {
  return fmt::format("link#{}", i);
}

static inline std::string debugInfo(const Net &net) {
  return "net";
}

static bool validateLinkSource(const LinkEnd &source,
                               const size_t i,
                               diag::Logger &logger) {
  bool passed = true;
  VALIDATE_GROUP_BEGIN(logger, debugInfo(source, i));

  VALIDATE(logger, (source.isValid() && source.getCellID() != OBJ_NULL_ID),
      "Unconnected link source");

  const auto &cell = source.getCell();
  const auto &type = cell.getType();
  VALIDATE(logger, (source.getPort() < type.getOutNum()),
      "Incorrect source pin: " << source.getPort() <<
      ", source cell has " << type.getOutNum() << " output pins");

  VALIDATE_GROUP_END(logger);
  return passed;
}

static bool validateCell(const CellID &cellID, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_GROUP_BEGIN(logger, debugInfo(cellID));

  const auto &cell = Cell::get(cellID);
  const auto &type = cell.getType();
  VALIDATE_QUIET(validateCellType(type, logger));
  VALIDATE(logger, (!type.isInNumFixed() || cell.getFanin() == type.getInNum()),
      "Incorrect number of inputs: " << cell.getFanin() <<
      ", expected " << type.getInNum());

  const auto links = cell.getLinks();
  VALIDATE(logger, (links.size() == cell.getFanin()),
      "Incorrect number of links: " << links.size() <<
      ", expected " << cell.getFanin());

  for (uint16_t i = 0; i < links.size(); ++i) {
    VALIDATE_QUIET(validateLinkSource(links[i], i, logger));
  }

  VALIDATE_GROUP_END(logger);
  return passed;
}

static bool validateCells(const List<CellID> &cells, diag::Logger &logger) {
  bool passed = true;
  for (auto i = cells.begin(); i != cells.end(); ++i) {
    VALIDATE_QUIET(validateCell(*i, logger));
  }
  return passed;
}

bool validateNet(const Net &net, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_GROUP_BEGIN(logger, debugInfo(net));

  VALIDATE(logger, (net.getInNum() > 0), "No inputs");
  VALIDATE(logger, (net.getOutNum() > 0), "No outputs");

  VALIDATE_QUIET(validateCells(net.getInputs(), logger));
  VALIDATE_QUIET(validateCells(net.getOutputs(), logger));
  VALIDATE_QUIET(validateCells(net.getCombCells(), logger));
  VALIDATE_QUIET(validateCells(net.getFlipFlops(), logger));
  VALIDATE_QUIET(validateCells(net.getSoftBlocks(), logger));
  VALIDATE_QUIET(validateCells(net.getHardBlocks(), logger));

  VALIDATE_GROUP_END(logger);
  return passed;
}

bool validateNet(const NetID netID, diag::Logger &logger) {
  bool passed = true;
  VALIDATE(logger, (netID != OBJ_NULL_ID), "Null net identifier");
  return passed && validateNet(Net::get(netID), logger);
}

//===----------------------------------------------------------------------===//
// Subnet Validator
//===----------------------------------------------------------------------===//

static inline std::string debugInfo(const Subnet::Cell &cell, const size_t i) {
  const auto &type = cell.getType();
  return fmt::format("cell#{}:{}", i, type.getName());
}

static inline std::string debugInfo(const Subnet::Link &link, const size_t i) {
  return fmt::format("link#{}", i);
}

static inline std::string debugInfo(const Subnet &subnet) {
  return "subnet";
}

static inline std::string debugInfo(const SubnetBuilder &builder) {
  return "subnet-builder";
}

static bool validateCell(const Subnet::Cell &cell,
                         const size_t entryID,
                         diag::Logger &logger) {
  bool passed = true;
  VALIDATE_GROUP_BEGIN(logger, debugInfo(cell, entryID));

  const auto &type = cell.getType();
  VALIDATE_QUIET(validateCellType(type, logger));
  VALIDATE(logger,
      (!type.isInNumFixed() || cell.getInNum() == type.getInNum()),
      "Incorrect number of input pins: " << cell.getInNum() <<
      ", expected " << type.getInNum());
  VALIDATE(logger,
      (!type.isOutNumFixed() || cell.getOutNum() == type.getOutNum()),
      "Incorrect number of output pins: " << cell.getOutNum() <<
      ", expected " << type.getOutNum());

  VALIDATE_GROUP_END(logger);
  return passed;
}

static bool validateCell(const Subnet::Cell &cell,
                         const size_t entryID,
                         const Subnet::LinkList &links,
                         const bool isTechMapped,
                         diag::Logger &logger) {
  bool passed = true;
  VALIDATE_GROUP_BEGIN(logger, debugInfo(cell, entryID));
  
  VALIDATE_QUIET(validateCell(cell, entryID, logger));

  const auto &type = cell.getType();
  if (!type.isIn() && !type.isOut()) {
    const auto isTechCell = type.isHard() || type.isCell();
    VALIDATE(logger, (isTechCell == isTechMapped),
       "Incorrect " << debugInfo(type) << ", expected a technology-" <<
       (isTechMapped ? "" : "in") << "dependent one");
  }

  for (size_t i = 0; i < links.size(); ++i) {
    VALIDATE(logger, (!isTechMapped || !links[i].inv),
        "Invertor " << debugInfo(links[i], i) <<
        " (logical gate NOT) in a tech-mapped subnet");
  }

  VALIDATE_GROUP_END(logger);
  return passed;
}

bool validateSubnet(const Subnet &subnet, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_GROUP_BEGIN(logger, debugInfo(subnet));

  const auto isTechMapped = subnet.isTechMapped();
  const auto &entries = subnet.getEntries();

  for (size_t i = 0; i < entries.size(); ++i) {
    const auto &cell = entries[i].cell;
    const auto links = subnet.getLinks(i);

    VALIDATE_QUIET(validateCell(cell, i, links, isTechMapped, logger));
    i += cell.more;
  }
 
  VALIDATE_GROUP_END(logger);
  return passed;
}

bool validateSubnet(const SubnetBuilder &builder, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_GROUP_BEGIN(logger, debugInfo(builder));

  const auto isTechMapped = builder.isTechMapped();

  for (auto i = builder.begin(); i != builder.end(); i.nextCell()) {
    const auto &cell = builder.getCell(*i);
    const auto links = builder.getLinks(*i);

    VALIDATE_QUIET(validateCell(cell, *i, links, isTechMapped, logger));
  }
 
  VALIDATE_GROUP_END(logger);
  return passed;
}

bool validateSubnet(const SubnetID subnetID, diag::Logger &logger) {
  bool passed = true;
  VALIDATE(logger, (subnetID != OBJ_NULL_ID), "Null subnet identifier");
  return passed && validateSubnet(Subnet::get(subnetID), logger);
}

//===----------------------------------------------------------------------===//
// Design Validator
//===----------------------------------------------------------------------===//

static inline std::string debugInfo(const DesignBuilder &builder) {
  return "design-builder";
}

bool validateDesign(const DesignBuilder &builder, diag::Logger &logger) {
  bool passed = true;
  VALIDATE_GROUP_BEGIN(logger, debugInfo(builder));

  for (size_t i = 0; i < builder.getSubnetNum(); ++i) {
    const auto &entry = builder.getEntry(i);
    VALIDATE(logger,
        (entry.subnetID != OBJ_NULL_ID) != (entry.builder != nullptr),
        "Inconsistent subnet");

    if (entry.subnetID != OBJ_NULL_ID) {
      VALIDATE_QUIET(validateSubnet(entry.subnetID, logger));
    } else {
      VALIDATE_QUIET(validateSubnet(*entry.builder, logger));
    }
  }

  VALIDATE_GROUP_END(logger);
  return passed;
}

} // namespace eda::gate::model
