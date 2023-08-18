//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"
#include "gate/model2/storage.h"
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
  NOP,
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

class CellType final {
public:
  using ID = CellTypeID;

  CellType(const std::string &name,
           NetID netID,
           CellTypeAttrID attrID,
           CellSymbol symbol,
           CellProperties props,
           uint16_t nIn,
           uint16_t nOut):
    nameID(allocate<String>(name)),
    netID(netID),
    attrID(attrID),
    symbol(symbol),
    props(props),
    nIn(nIn),
    nOut(nOut) {}

  const String &getName() const { return *access<String>(nameID); }

  CellSymbol getSymbol() const { return symbol; }

  bool isCombinational() const { return props.combinational; }
  bool isConstant()      const { return props.constant;      }
  bool isIdentity()      const { return props.identity;      }
  bool isCommutative()   const { return props.commutative;   }
  bool isAssociative()   const { return props.associative;   }

  uint16_t getInNumber()  const { return nIn;  }
  uint16_t getOutNumber() const { return nOut; }

private:
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
// Standard Cell Types
//===----------------------------------------------------------------------===//

// Full cell type identifiers.
extern const CellTypeID CELL_TYPE_ID_IN;
extern const CellTypeID CELL_TYPE_ID_OUT;
extern const CellTypeID CELL_TYPE_ID_ZERO;
extern const CellTypeID CELL_TYPE_ID_ONE;
extern const CellTypeID CELL_TYPE_ID_NOP;
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

// Short cell type identifiers.
extern const uint32_t CELL_TYPE_SID_IN;
extern const uint32_t CELL_TYPE_SID_OUT;
extern const uint32_t CELL_TYPE_SID_ZERO;
extern const uint32_t CELL_TYPE_SID_ONE;
extern const uint32_t CELL_TYPE_SID_NOP;
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

} // namespace eda::gate::model
