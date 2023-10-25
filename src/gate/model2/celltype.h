//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"
#include "gate/model2/string.h"

#include <cstdint>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Cell Symbol
//===----------------------------------------------------------------------===//

enum CellSymbol : uint16_t {
  /// Input.
  IN,
  /// Output.
  OUT,

  /// Constant 0: OUT = 0.
  ZERO,
  /// Constant 1: OUT = 1.
  ONE,

  /// Identity: OUT = X.
  BUF,
  /// Negation: OUT = ~X.
  NOT,

  /// Conjunction: OUT = X & Y (& ...).
  AND,
  /// Disjunction: OUT = X | Y (| ...).
  OR,
  /// Exclusive OR: OUT = X + Y (+ ...) (mod 2).
  XOR,
  /// Sheffer's stroke: OUT = ~(X & Y (& ...)).
  NAND,
  /// Peirce's arrow: OUT = ~(X | Y (| ...)).
  NOR,
  /// Exclusive NOR: OUT = ~(X + Y (+ ...) (mod 2)).
  XNOR,
  /// Majority function: OUT = Majority(X, Y, ...).
  MAJ,

  /// D latch (Q, D, ENA):
  /// Q(t) = ENA(level1) ? D : Q(t-1).
  LATCH,
  /// D flip-flop (Q, D, CLK):
  /// Q(t) = CLK(posedge) ? D : Q(t-1).
  DFF,
  /// D flip-flop w/ (asynchronous) reset and set (Q, D, CLK, RST, SET):
  /// Q(t) = RST(level1) ? 0 : (SET(level1) ? 1 : (CLK(posedge) ? D : Q(t-1))).
  DFFrs,

  /// Standard cell.
  CELL,
  /// Macrocell.
  NET,
  /// Soft IP core.
  SOFT,
  /// Hard IP core.
  HARD
};

static_assert(sizeof(CellSymbol) == 2);

//===----------------------------------------------------------------------===//
// Cell Properties
//===----------------------------------------------------------------------===//

#pragma pack(push, 1)
struct CellProperties {
  CellProperties(bool combinational,
                 bool constant,
                 bool identity,
                 bool commutative,
                 bool associative):
    combinational(combinational),
    constant(constant),
    identity(identity),
    commutative(commutative),
    associative(associative) {}

  unsigned combinational : 1;
  unsigned constant      : 1;
  unsigned identity      : 1;
  unsigned commutative   : 1;
  unsigned associative   : 1;

  unsigned reserved      : 11;
};
#pragma pack(pop)

static_assert(sizeof(CellProperties) == 2);

//===----------------------------------------------------------------------===//
// Cell Type
//===----------------------------------------------------------------------===//
class Net;

class CellType final : public Object<CellType, CellTypeID> {
  friend class Storage<CellType>;

public:
  static constexpr uint16_t AnyArity = 0xffff;

  std::string getName() const { return String::get(nameID); }

  CellSymbol getSymbol() const { return symbol; }

  bool isCombinational() const { return props.combinational; }
  bool isConstant()      const { return props.constant;      }
  bool isIdentity()      const { return props.identity;      }
  bool isCommutative()   const { return props.commutative;   }
  bool isAssociative()   const { return props.associative;   }

  uint16_t getInNum()  const { return nIn;  }
  uint16_t getOutNum() const { return nOut; }

  bool isAnyArity() const { return nIn == AnyArity; }

  bool isNet() const { return netID != OBJ_NULL_ID; }
  const Net &getNet() const;

private:
  CellType(const std::string &name,
           NetID netID,
           CellTypeAttrID attrID,
           CellSymbol symbol,
           CellProperties props,
           uint16_t nIn,
           uint16_t nOut):
    nameID(makeString(name)),
    netID(netID),
    attrID(attrID),
    symbol(symbol),
    props(props),
    nIn(nIn),
    nOut(nOut) {}

  const StringID nameID;

  const NetID netID;
  const CellTypeAttrID attrID;

  const CellSymbol symbol;
  const CellProperties props;

  const uint16_t nIn;
  const uint16_t nOut;
};

static_assert(sizeof(CellType) == CellTypeID::Size);

//===----------------------------------------------------------------------===//
// Cell Type Builder
//===----------------------------------------------------------------------===//

inline CellTypeID makeCellType(const std::string &name,
                               NetID netID,
                               CellTypeAttrID attrID,
                               CellSymbol symbol,
                               CellProperties props,
                               uint16_t nIn,
                               uint16_t nOut) {
  return allocate<CellType>(name, netID, attrID, symbol, props, nIn, nOut);
}

inline CellTypeID makeCellType(const std::string &name,
                               CellSymbol symbol,
                               CellProperties props,
                               uint16_t nIn,
                               uint16_t nOut) {
  return makeCellType(name, OBJ_NULL_ID, OBJ_NULL_ID, symbol, props, nIn, nOut);
}

//===----------------------------------------------------------------------===//
// Standard Cell Types
//===----------------------------------------------------------------------===//

// Full cell type identifiers.
extern const CellTypeID CELL_TYPE_ID_IN;
extern const CellTypeID CELL_TYPE_ID_OUT;
extern const CellTypeID CELL_TYPE_ID_ZERO;
extern const CellTypeID CELL_TYPE_ID_ONE;
extern const CellTypeID CELL_TYPE_ID_BUF;
extern const CellTypeID CELL_TYPE_ID_NOT;
extern const CellTypeID CELL_TYPE_ID_AND;
extern const CellTypeID CELL_TYPE_ID_OR;
extern const CellTypeID CELL_TYPE_ID_XOR;
extern const CellTypeID CELL_TYPE_ID_NAND;
extern const CellTypeID CELL_TYPE_ID_NOR;
extern const CellTypeID CELL_TYPE_ID_XNOR;
extern const CellTypeID CELL_TYPE_ID_MAJ;
extern const CellTypeID CELL_TYPE_ID_LATCH;
extern const CellTypeID CELL_TYPE_ID_DFF;
extern const CellTypeID CELL_TYPE_ID_DFFrs;

constexpr uint64_t getCellTypeID(CellSymbol symbol) {
  switch(symbol) {
  case IN:    return CELL_TYPE_ID_IN;
  case OUT:   return CELL_TYPE_ID_OUT;
  case ZERO:  return CELL_TYPE_ID_ZERO;
  case ONE:   return CELL_TYPE_ID_ONE;
  case BUF:   return CELL_TYPE_ID_BUF;
  case NOT:   return CELL_TYPE_ID_NOT;
  case AND:   return CELL_TYPE_ID_AND;
  case OR:    return CELL_TYPE_ID_OR;
  case XOR:   return CELL_TYPE_ID_XOR;
  case NAND:  return CELL_TYPE_ID_NAND;
  case NOR:   return CELL_TYPE_ID_NOR;
  case XNOR:  return CELL_TYPE_ID_XNOR;
  case MAJ:   return CELL_TYPE_ID_MAJ;
  case LATCH: return CELL_TYPE_ID_LATCH;
  case DFF:   return CELL_TYPE_ID_DFF;
  case DFFrs: return CELL_TYPE_ID_DFFrs;
  default:    return OBJ_NULL_ID;
  }
}

// Short cell type identifiers.
extern const uint32_t CELL_TYPE_SID_IN;
extern const uint32_t CELL_TYPE_SID_OUT;
extern const uint32_t CELL_TYPE_SID_ZERO;
extern const uint32_t CELL_TYPE_SID_ONE;
extern const uint32_t CELL_TYPE_SID_BUF;
extern const uint32_t CELL_TYPE_SID_NOT;
extern const uint32_t CELL_TYPE_SID_AND;
extern const uint32_t CELL_TYPE_SID_OR;
extern const uint32_t CELL_TYPE_SID_XOR;
extern const uint32_t CELL_TYPE_SID_NAND;
extern const uint32_t CELL_TYPE_SID_NOR;
extern const uint32_t CELL_TYPE_SID_XNOR;
extern const uint32_t CELL_TYPE_SID_MAJ;
extern const uint32_t CELL_TYPE_SID_LATCH;
extern const uint32_t CELL_TYPE_SID_DFF;
extern const uint32_t CELL_TYPE_SID_DFFrs;

constexpr uint32_t getCellTypeSID(CellSymbol symbol) {
  switch(symbol) {
  case IN:    return CELL_TYPE_SID_IN;
  case OUT:   return CELL_TYPE_SID_OUT;
  case ZERO:  return CELL_TYPE_SID_ZERO;
  case ONE:   return CELL_TYPE_SID_ONE;
  case BUF:   return CELL_TYPE_SID_BUF;
  case NOT:   return CELL_TYPE_SID_NOT;
  case AND:   return CELL_TYPE_SID_AND;
  case OR:    return CELL_TYPE_SID_OR;
  case XOR:   return CELL_TYPE_SID_XOR;
  case NAND:  return CELL_TYPE_SID_NAND;
  case NOR:   return CELL_TYPE_SID_NOR;
  case XNOR:  return CELL_TYPE_SID_XNOR;
  case MAJ:   return CELL_TYPE_SID_MAJ;
  case LATCH: return CELL_TYPE_SID_LATCH;
  case DFF:   return CELL_TYPE_SID_DFF;
  case DFFrs: return CELL_TYPE_SID_DFFrs;
  default:    return -1u;
  }
}

} // namespace eda::gate::model
