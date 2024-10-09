//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/cellattr.h"
#include "gate/model/object.h"
#include "gate/model/storage.h"
#include "gate/model/string.h"

#include <cstdint>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Cell Symbol
//===----------------------------------------------------------------------===//

enum CellSymbolFlags : uint8_t {
  PNEDGE_BIT = 15,
  ENALVL_BIT = 15, // CLK in DFF | ENA in LATCH
  RSTLVL_BIT = 14,
  SETLVL_BIT = 13,
  RSTVAL_BIT = SETLVL_BIT
};

static constexpr uint16_t PNEDGE_MASK = (1u << PNEDGE_BIT);
static constexpr uint16_t ENALVL_MASK = (1u << ENALVL_BIT);
static constexpr uint16_t RSTLVL_MASK = (1u << RSTLVL_BIT);
static constexpr uint16_t SETLVL_MASK = (1u << SETLVL_BIT);
static constexpr uint16_t RSTVAL_MASK = (1u << RSTVAL_BIT);

static constexpr uint16_t FLGMASK = PNEDGE_MASK
                                  | ENALVL_MASK
                                  | RSTLVL_MASK
                                  | SETLVL_MASK
                                  | RSTVAL_MASK;

static constexpr uint16_t POSEDGE = (0u << PNEDGE_BIT); // Default
static constexpr uint16_t NEGEDGE = (1u << PNEDGE_BIT);
static constexpr uint16_t ENALVL1 = (0u << ENALVL_BIT); // Default
static constexpr uint16_t ENALVL0 = (1u << ENALVL_BIT);
static constexpr uint16_t RSTLVL1 = (0u << RSTLVL_BIT); // Default
static constexpr uint16_t RSTLVL0 = (1u << RSTLVL_BIT);
static constexpr uint16_t SETLVL1 = (0u << SETLVL_BIT); // Default
static constexpr uint16_t SETLVL0 = (1u << SETLVL_BIT);
static constexpr uint16_t RSTVAL0 = (0u << RSTVAL_BIT); // Default
static constexpr uint16_t RSTVAL1 = (1u << RSTVAL_BIT);

inline bool getClkEdge(uint16_t symbol) {
  return (symbol & PNEDGE_MASK) == POSEDGE;
}

inline bool getEnaLevel(uint16_t symbol) {
  return (symbol & ENALVL_MASK) == ENALVL1;
}

inline bool getRstLevel(uint16_t symbol) {
  return (symbol & RSTLVL_MASK) == RSTLVL1;
}

inline bool getSetLevel(uint16_t symbol) {
  return (symbol & SETLVL_MASK) == SETLVL1;
}

inline bool getRstValue(uint16_t symbol) {
  return (symbol & RSTVAL_MASK) == RSTVAL1;
}

enum CellSymbol : uint16_t {
  //===-------------------------- Inputs/Outputs --------------------------===//

  /// Input.
  IN,
  /// Output.
  OUT,

  //===----------------------------- Constants ----------------------------===//

  /// Constant 0: OUT = 0.
  ZERO,
  /// Constant 1: OUT = 1.
  ONE,

  //===---------------------------- Unary Gates ---------------------------===//

  /// Identity: OUT = X.
  BUF,
  /// Negation: OUT = ~X.
  NOT,

  //===-------------------------- Multiarity Gates ------------------------===//

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

  //===---------------------------- Flip-Flops ----------------------------===//

  /// D flip-flop (Q, D, CLK):
  /// Q(t) = CLK(posedge) ? D : Q(t-1).
  DFF,
  /// D flip-flop w/ synchronous reset (Q, D, CLK, RST):
  /// Q(t) = CLK(posedge) ? (RST ? 0 : D) : Q(t-1).
  sDFF,
  /// D flip-flop w/ asynchronous reset (Q, D, CLK, RST):
  /// Q(t) = RST(level=1) ? 0 : (CLK(posedge) ? D : Q(t-1)).
  aDFF,
  /// D flip-flop w/ (asynchronous) reset and set (Q, D, CLK, RST, SET):
  /// Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : (CLK(posedge) ? D : Q(t-1))).
  DFFrs,

  //===------------------------------ Latches -----------------------------===//

  /// D latch (Q, D, ENA):
  /// Q(t) = ENA(level=1) ? D : Q(t-1).
  DLATCH,
  /// D latch w/ asynchronous reset (Q, D, ENA, RST):
  /// Q(t) = RST(level=1) ? 0 : (ENA(level=1) ? D : Q(t-1)).
  aDLATCH,
  /// D latch w/ (asynchronous) reset and set (Q, D, ENA, RST, SET):
  /// Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : (ENA(level=1) ? D : Q(t-1))).
  DLATCHrs,
  /// RS latch (Q, RST, SET):
  /// Q(t) = RST(level=1) ? 0 : (SET(level=1) ? 1 : Q(t-1)).
  LATCHrs,

  //===------------------------ Bitwise Operations ------------------------===//

  /// Bitwise NOT (signed): OUT = ~X.
  BNOTs,
  /// Bitwise NOT (unsigned): OUT = ~X.
  BNOTu,
  BNOT = BNOTu,
  /// Bitwise AND (signed): OUT = X & Y.
  BANDs,
  /// Bitwise AND (unsigned): OUT = X & Y.
  BANDu,
  BAND = BANDu,
  /// Bitwise OR (signed): OUT = X | Y.
  BORs,
  /// Bitwise OR (unsigned): OUT = X | Y.
  BORu,
  BOR = BORu,
  /// Bitwise XOR (signed): OUT = X + Y (mod 2).
  BXORs,
  /// Bitwise XOR (unsigned): OUT = X + Y (mod 2).
  BXORu,
  BXOR = BXORu,
  /// Bitwise NAND (signed): OUT = ~(X & Y).
  BNANDs,
  /// Bitwise NAND (unsigned): OUT = ~(X & Y).
  BNANDu,
  BNAND = BNANDu,
  /// Bitwise NOR (signed): OUT = ~(X | Y).
  BNORs,
  /// Bitwise NOR (unsigned): OUT = ~(X | Y).
  BNORu,
  BNOR = BNORu,
  /// Bitwise XNOR (signed): OUT = ~(X + Y) (mod 2).
  BXNORs,
  /// Bitwise XNOR (unsigned): OUT = ~(X + Y) (mod 2).
  BXNORu,
  BXNOR = BXNORu,

  //===------------------------- Reduce Operations ------------------------===//

  /// Reduction AND: OUT = X[1] & ... & X[n].
  RAND,
  /// Reduction OR: OUT = X[1] | ... | X[n].
  ROR,
  /// Reduction XOR: OUT = (X[1] + ... + X[n]) (mod 2).
  RXOR,
  /// Reduction NAND: OUT = ~(X[1] & ... & X[n]).
  RNAND,
  /// Reduction NOR: OUT = ~(X[1] | ... | X[n]).
  RNOR,
  /// Reduction XNOR: OUT = ~(X[1] + ... + X[n]) (mod 2).
  RXNOR,

  //===------------------ Multiplexors / Demultiplexors -------------------===//

  /// Bitwise MUX 2-to-1 (S[*], X[*], Y[*]), |S| == |X| == |Y|:
  /// OUT[i] = (S[i] == 0) ? X[i] : Y[i].
  BMUX,
  /// Multibit MUX 2-to-1 (S, X[*], Y[*]):
  /// OUT[i] = (S == 0) ? X[i] : Y[i].
  MUX2,
  /// Multibit MUX *-to-1 (S[*], X[*]), |X| == |OUT| * 2^|S|:
  /// OUT[i] = X[INDEX(S) * |OUT| + i].
  MUX,
  /// Bitwise DEMUX 1-to-2 (S[*], X[*]):
  /// OUT[s][i] = (S[i] == s) ? X[i] : 0, s=0,1.
  BDEMUX,
  /// Multibit DEMUX 1-to-2 (S, X[*]):
  /// OUT[s][i] = (S == s) ? X[i] : 0, s=0,1.
  DEMUX2,
  /// Multibit DEMUX 1-to-* (S[*], X[*]), |OUT| == |X| * 2^|S|:
  /// OUT[i] = ((i / |X|) == INDEX(S)) ? X[i % |X|] : 0.
  DEMUX,

  //===------------------------- Shift Operations -------------------------===//

  /// Shift left (signed [X is signed, Y is unsigned]):
  /// OUT = X << Y.
  SHLs,
  /// Shift left (unsigned [X is unsigned, Y is unsigned]):
  /// OUT = X << Y.
  SHLu,
  SHL = SHLu,
  /// Shift right (signed [X is signed, Y is unsigned]):
  /// OUT = X >> Y.
  SHRs,
  /// Shift right (unsigned [X is unsigned, Y is unsigned]):
  /// OUT = X >> Y.
  SHRu,
  SHR = SHRu,
  /// Shift (signed [X is signed, Y is signed]):
  /// OUT = (Y >= 0) ? SHRs(X, Y) : SHLs(X, -Y).
  SHIFTs,
  /// Shift (unsigned [X is unsigned, Y is signed]):
  /// OUT = (Y >= 0) ? SHRu(X, Y) : SHLu(X, -Y).
  SHIFTu,
  SHIFT = SHIFTu,

  //===----------------------- Comparison Operations ----------------------===//

  /// Equality comparison (signed): OUT = X == Y.
  EQs,
  /// Equality comparison (unsigned): OUT = X == Y.
  EQu,
  EQ = EQu,
  /// Non-equality comparison (signed): OUT = X != Y.
  NEQs,
  /// Non-equality comparison (unsigned): OUT = X != Y.
  NEQu,
  NEQ = NEQu,

  /// Equality comparison (signed): OUT = X === Y (non-synthesizable).
  EQXs,
  /// Equality comparison (unsigned): OUT = X === Y (non-synthesizable).
  EQXu,
  EQX = EQXu,
  /// Non-equality comparison (signed): OUT = X !== Y (non-synthesizable).
  NEQXs,
  /// Non-equality comparison (unsigned): OUT = X !== Y (non-synthesizable).
  NEQXu,
  NEQX = NEQXu,

  /// Less-than comparison (signed): OUT = X < Y.
  LTs,
  /// Less-than comparison (unsigned): OUT = X < Y.
  LTu,
  LT = LTu,
  /// Less-than-equal comparison (signed): OUT = X <= Y.
  LTEs,
  /// Less-than-equal comparison (unsigned): OUT = X <= Y.
  LTEu,
  LTE = LTEu,
  /// Greater-than comparison (signed): OUT = X > Y.
  GTs,
  /// Greater-than comparison (unsigned): OUT = X > Y.
  GTu,
  GT = GTu,
  /// Greater-than-equal comparison (signed): OUT = X >= Y.
  GTEs,
  /// Greater-than-equal comparison (unsigned): OUT = X >= Y.
  GTEu,
  GTE = GTEu,

  //===----------------------- Arithmetic Operations ----------------------===//

  /// Negation (signed): OUT = -X.
  NEGs,
  /// Negation (unsigned): OUT = -X.
  NEGu,
  NEG = NEGu,
  /// Addition (signed): OUT = X + Y.
  ADDs,
  /// Addition (unsigned): OUT = X + Y.
  ADDu,
  ADD = ADDu,
  /// Subtraction (signed): OUT = X - Y.
  SUBs,
  /// Subtraction (unsigned): OUT = X - Y.
  SUBu,
  SUB = SUBu,

  /// Multiplication (signed): OUT = X * Y.
  MULs,
  /// Multiplication (unsigned): OUT = X * Y.
  MULu,
  MUL = MULu,
  /// Division (signed): OUT = X / Y.
  DIVs,
  /// Division (unsigned): OUT = X / Y.
  DIVu,
  DIV = DIVu,
  /// Remainder (signed): OUT = X rem Y = sign(X)*(|X| rem |Y|).
  REMs,
  /// Remainder (unsigned): OUT = X rem Y.
  REMu,
  REM = REMu,
  /// Modulo (signed): OUT = X mod Y = sign(Y)*(|X| rem |Y|).
  MODs,

  //===--------------------- Flip-Flop Specializations --------------------===//

  /// DFF[CLK(posedge)] = DFF.
  DFF_p = (DFF | POSEDGE),
  /// DFF[CLK(negedge)].
  DFF_n = (DFF | NEGEDGE),

  /// sDFF[CLK(posedge), RST(level=1), RST(value=0)] = sDFF.
  sDFF_pp0 = (sDFF | POSEDGE | RSTLVL1 | RSTVAL0),
  /// sDFF[CLK(posedge), RST(level=1), RST(value=1)].
  sDFF_pp1 = (sDFF | POSEDGE | RSTLVL1 | RSTVAL1),
  /// sDFF[CLK(posedge), RST(level=0), RST(value=0)].
  sDFF_pn0 = (sDFF | POSEDGE | RSTLVL0 | RSTVAL0),
  /// sDFF[CLK(posedge), RST(level=0), RST(value=1)].
  sDFF_pn1 = (sDFF | POSEDGE | RSTLVL0 | RSTVAL1),
  /// sDFF[CLK(negedge), RST(level=1), RST(value=0)].
  sDFF_np0 = (sDFF | NEGEDGE | RSTLVL1 | RSTVAL0),
  /// sDFF[CLK(negedge), RST(level=1), RST(value=1)].
  sDFF_np1 = (sDFF | NEGEDGE | RSTLVL1 | RSTVAL1),
  /// sDFF[CLK(negedge), RST(level=0), RST(value=0)].
  sDFF_nn0 = (sDFF | NEGEDGE | RSTLVL0 | RSTVAL0),
  /// sDFF[CLK(negedge), RST(level=0), RST(value=1)].
  sDFF_nn1 = (sDFF | NEGEDGE | RSTLVL0 | RSTVAL1),

  /// aDFF[CLK(posedge), RST(level=1), RST(value=0)] = aDFF.
  aDFF_pp0 = (aDFF | POSEDGE | RSTLVL1 | RSTVAL0),
  /// aDFF[CLK(posedge), RST(level=1), RST(value=1)].
  aDFF_pp1 = (aDFF | POSEDGE | RSTLVL1 | RSTVAL1),
  /// aDFF[CLK(posedge), RST(level=0), RST(value=0)].
  aDFF_pn0 = (aDFF | POSEDGE | RSTLVL0 | RSTVAL0),
  /// aDFF[CLK(posedge), RST(level=0), RST(value=1)].
  aDFF_pn1 = (aDFF | POSEDGE | RSTLVL0 | RSTVAL1),
  /// aDFF[CLK(negedge), RST(level=1), RST(value=0)].
  aDFF_np0 = (aDFF | NEGEDGE | RSTLVL1 | RSTVAL0),
  /// aDFF[CLK(negedge), RST(level=1), RST(value=1)].
  aDFF_np1 = (aDFF | NEGEDGE | RSTLVL1 | RSTVAL1),
  /// aDFF[CLK(negedge), RST(level=0), RST(value=0)].
  aDFF_nn0 = (aDFF | NEGEDGE | RSTLVL0 | RSTVAL0),
  /// aDFF[CLK(negedge), RST(level=0), RST(value=1)].
  aDFF_nn1 = (aDFF | NEGEDGE | RSTLVL0 | RSTVAL1),

  /// DFFrs[CLK(posedge), RST(level=1), SET(level=1)] = DFFrs.
  DFFrs_ppp = (DFFrs | POSEDGE | RSTLVL1 | SETLVL1),
  /// DFFrs[CLK(posedge), RST(level=1), SET(level=0)].
  DFFrs_ppn = (DFFrs | POSEDGE | RSTLVL1 | SETLVL0),
  /// DFFrs[CLK(posedge), RST(level=0), SET(level=1)].
  DFFrs_pnp = (DFFrs | POSEDGE | RSTLVL0 | SETLVL1),
  /// DFFrs[CLK(posedge), RST(level=0), SET(level=0)].
  DFFrs_pnn = (DFFrs | POSEDGE | RSTLVL0 | SETLVL0),
  /// DFFrs[CLK(negedge), RST(level=1), SET(level=1)].
  DFFrs_npp = (DFFrs | NEGEDGE | RSTLVL1 | SETLVL1),
  /// DFFrs[CLK(negedge), RST(level=1), SET(level=0)].
  DFFrs_npn = (DFFrs | NEGEDGE | RSTLVL1 | SETLVL0),
  /// DFFrs[CLK(negedge), RST(level=0), SET(level=1)].
  DFFrs_nnp = (DFFrs | NEGEDGE | RSTLVL0 | SETLVL1),
  /// DFFrs[CLK(negedge), RST(level=0), SET(level=0)].
  DFFrs_nnn = (DFFrs | NEGEDGE | RSTLVL0 | SETLVL0),

  //===----------------------- Latch Specializations ----------------------===//

  /// DLATCH[ENA(level=1)] = DLATCH.
  DLATCH_p = (DLATCH | ENALVL1),
  /// DLATCH[ENA(level=0)].
  DLATCH_n = (DLATCH | ENALVL0),

  /// aDLATCH[ENA(level=1), RST(level=1), RST(value=0)] = aDLATCH.
  aDLATCH_pp0 = (aDLATCH | ENALVL1 | RSTLVL1 | RSTVAL0),
  /// aDLATCH[ENA(level=1), RST(level=1), RST(value=1)].
  aDLATCH_pp1 = (aDLATCH | ENALVL1 | RSTLVL1 | RSTVAL1),
  /// aDLATCH[ENA(level=1), RST(level=0), RST(value=0)].
  aDLATCH_pn0 = (aDLATCH | ENALVL1 | RSTLVL0 | RSTVAL0),
  /// aDLATCH[ENA(level=1), RST(level=0), RST(value=1)].
  aDLATCH_pn1 = (aDLATCH | ENALVL1 | RSTLVL0 | RSTVAL1),
  /// aDLATCH[ENA(level=0), RST(level=1), RST(value=0)].
  aDLATCH_np0 = (aDLATCH | ENALVL0 | RSTLVL1 | RSTVAL0),
  /// aDLATCH[ENA(level=0), RST(level=1), RST(value=1)].
  aDLATCH_np1 = (aDLATCH | ENALVL0 | RSTLVL1 | RSTVAL1),
  /// aDLATCH[ENA(level=0), RST(level=0), RST(value=0)].
  aDLATCH_nn0 = (aDLATCH | ENALVL0 | RSTLVL0 | RSTVAL0),
  /// aDLATCH[ENA(level=0), RST(level=0), RST(value=1)].
  aDLATCH_nn1 = (aDLATCH | ENALVL0 | RSTLVL0 | RSTVAL1),

  /// DLATCHrs[ENA(level=1), RST(level=1), SET(level=1)] = DLATCHrs.
  DLATCHrs_ppp = (DLATCHrs | ENALVL1 | RSTLVL1 | SETLVL1),
  /// DLATCHrs[ENA(level=1), RST(level=1), SET(level=0)].
  DLATCHrs_ppn = (DLATCHrs | ENALVL1 | RSTLVL1 | SETLVL0),
  /// DLATCHrs[ENA(level=1), RST(level=0), SET(level=1)].
  DLATCHrs_pnp = (DLATCHrs | ENALVL1 | RSTLVL0 | SETLVL1),
  /// DLATCHrs[ENA(level=1), RST(level=0), SET(level=0)].
  DLATCHrs_pnn = (DLATCHrs | ENALVL1 | RSTLVL0 | SETLVL0),
  /// DLATCHrs[ENA(level=0), RST(level=1), SET(level=1)].
  DLATCHrs_npp = (DLATCHrs | ENALVL0 | RSTLVL1 | SETLVL1),
  /// DLATCHrs[ENA(level=0), RST(level=1), SET(level=0)].
  DLATCHrs_npn = (DLATCHrs | ENALVL0 | RSTLVL1 | SETLVL0),
  /// DLATCHrs[ENA(level=0), RST(level=0), SET(level=1)].
  DLATCHrs_nnp = (DLATCHrs | ENALVL0 | RSTLVL0 | SETLVL1),
  /// DLATCHrs[ENA(level=0), RST(level=0), SET(level=0)].
  DLATCHrs_nnn = (DLATCHrs | ENALVL0 | RSTLVL0 | SETLVL0),

  /// LATCHrs[RST(level=1), SET(level=1)] = LATCHrs.
  LATCHrs_pp = (LATCHrs | RSTLVL1 | SETLVL1),
  /// LATCHrs[RST(level=1), SET(level=0)].
  LATCHrs_pn = (LATCHrs | RSTLVL1 | SETLVL0),
  /// LATCHrs[RST(level=0), SET(level=1)].
  LATCHrs_np = (LATCHrs | RSTLVL0 | SETLVL1),
  /// LATCHrs[RST(level=0), SET(level=0)].
  LATCHrs_nn = (LATCHrs | RSTLVL0 | SETLVL0),

  //===------------------------- Custom Operations ------------------------===//

  /// Custom block.
  UNDEF = (0xffff & ~FLGMASK)
};

static_assert(sizeof(CellSymbol) == 2);

static_assert(DFF == DFF_p);
static_assert(sDFF == sDFF_pp0);
static_assert(aDFF == aDFF_pp0);
static_assert(DFFrs == DFFrs_ppp);
static_assert(DLATCH == DLATCH_p);
static_assert(aDLATCH == aDLATCH_pp0);
static_assert(DLATCHrs == DLATCHrs_ppp);
static_assert(LATCHrs == LATCHrs_pp);

enum CellPin : uint16_t {
  DFF_IN_D       = 0,
  DFF_IN_CLK     = 1,
  DFF_IN_RST     = 2,
  DFF_IN_SET     = 3,
  DLATCH_IN_D    = 0,
  DLATCH_IN_ENA  = 1,
  DLATCH_IN_RST  = 2,
  DLATCH_IN_SET  = 3,
  LATCHrs_IN_RST = 0,
  LATCHrs_IN_SET = 1
};

inline CellSymbol getNegSymbol(CellSymbol symbol) {
  switch (symbol) {
  case ZERO: return ONE;
  case ONE:  return ZERO;
  case BUF:  return NOT;
  case NOT:  return BUF;
  case AND:  return NAND;
  case OR:   return NOR;
  case XOR:  return XNOR;
  case NAND: return AND;
  case NOR:  return OR;
  case XNOR: return XOR;
  default: assert(false);
  }
  return UNDEF;
}

//===----------------------------------------------------------------------===//
// Cell Properties
//===----------------------------------------------------------------------===//

#pragma pack(push, 1)
struct CellProperties {
  CellProperties(bool cell,
                 bool soft,
                 bool combinational,
                 bool constant,
                 bool identity,
                 bool commutative,
                 bool associative,
                 bool regroupable,
                 bool negative):
    cell(cell),
    soft(soft),
    combinational(combinational),
    constant(constant),
    identity(identity),
    commutative(commutative),
    associative(associative),
    regroupable(regroupable),
    negative(negative),
    padding__(0) {}

  /// Cell/soft flags identify the cell kind:
  /// 00: Hard (block w/ unknown structure);
  /// 01: Soft (block w/ known structure);
  /// 10: Cell (technology-dependent cell);
  /// 11: Gate (elementary logic function).
  unsigned cell : 1;
  unsigned soft : 1;

  /// Describes the function properties.
  unsigned combinational : 1;
  unsigned constant      : 1;
  unsigned identity      : 1;
  unsigned commutative   : 1;
  unsigned associative   : 1;
  unsigned regroupable   : 1;
  unsigned negative      : 1;
  unsigned padding__     : 7;
};
#pragma pack(pop)

static_assert(sizeof(CellProperties) == 2);

//===----------------------------------------------------------------------===//
// Cell Type
//===----------------------------------------------------------------------===//
class Net;
class Subnet;

class CellType final : public Object<CellType, CellTypeID> {
  friend class Storage<CellType>;

public:
  using PortWidths = CellTypeAttr::PortWidths;
  using PortVector = CellTypeAttr::PortVector;

  static constexpr uint16_t AnyArity = 0xffff;
  static constexpr uint16_t MaxArity = 0xfffe;

  CellType &operator=(const CellType &r) = delete;
  CellType(const CellType &r) = delete;

  /// Returns the cell type name.
  std::string getName() const { return String::get(nameID); }
  /// Returns the cell type function/kind.
  CellSymbol getSymbol() const { return symbol; }

  bool isIn()     const { return symbol == IN;     }
  bool isOut()    const { return symbol == OUT;    }
  bool isZero()   const { return symbol == ZERO;   }
  bool isOne()    const { return symbol == ONE;    }
  bool isBuf()    const { return symbol == BUF;    }
  bool isNot()    const { return symbol == NOT;    }
  bool isAnd()    const { return symbol == AND;    }
  bool isOr()     const { return symbol == OR;     }
  bool isXor()    const { return symbol == XOR;    }
  bool isNand()   const { return symbol == NAND;   }
  bool isNor()    const { return symbol == NOR;    }
  bool isXnor()   const { return symbol == XNOR;   }
  bool isMaj()    const { return symbol == MAJ;    }
  bool isBNot()   const { return symbol == BNOT;   }
  bool isBAndS()  const { return symbol == BANDs;  }
  bool isBAndU()  const { return symbol == BANDu;  }
  bool isBOrS()   const { return symbol == BORs;   }
  bool isBOrU()   const { return symbol == BORu;   }
  bool isBXorS()  const { return symbol == BXORs;  }
  bool isBXorU()  const { return symbol == BXORu;  }
  bool isBNandS() const { return symbol == BNANDs; }
  bool isBNandU() const { return symbol == BNANDu; }
  bool isBNorS()  const { return symbol == BNORs;  }
  bool isBNorU()  const { return symbol == BNORu;  }
  bool isBXnorS() const { return symbol == BXNORs; }
  bool isBXnorU() const { return symbol == BXNORu; }
  bool isRAnd()   const { return symbol == RAND;   }
  bool isROr()    const { return symbol == ROR;    }
  bool isRXor()   const { return symbol == RXOR;   }
  bool isRNand()  const { return symbol == RNAND;  }
  bool isRNor()   const { return symbol == RNOR;   }
  bool isRXnor()  const { return symbol == RXNOR;  }
  bool isMux2()   const { return symbol == MUX2;   }
  bool isShl()    const { return symbol == SHL;    }
  bool isShrS()   const { return symbol == SHRs;   }
  bool isShrU()   const { return symbol == SHRu;   }
  bool isEqS()    const { return symbol == EQs;    }
  bool isEqU()    const { return symbol == EQu;    }
  bool isNeqS()   const { return symbol == NEQs;   }
  bool isNeqU()   const { return symbol == NEQu;   }
  bool isEqxS()   const { return symbol == EQXs;   }
  bool isEqxU()   const { return symbol == EQXu;   }
  bool isNeqxS()  const { return symbol == NEQXs;  }
  bool isNeqxU()  const { return symbol == NEQXu;  }
  bool isLtS()    const { return symbol == LTs;    }
  bool isLtU()    const { return symbol == LTu;    }
  bool isLteS()   const { return symbol == LTEs;   }
  bool isLteU()   const { return symbol == LTEu;   }
  bool isGtS()    const { return symbol == GTs;    }
  bool isGtU()    const { return symbol == GTu;    }
  bool isGteS()   const { return symbol == GTEs;   }
  bool isGteU()   const { return symbol == GTEu;   }
  bool isNeg()    const { return symbol == NEG;    }
  bool isAdd()    const { return symbol == ADD;    }
  bool isSub()    const { return symbol == SUB;    }
  bool isMulS()   const { return symbol == MULs;   }
  bool isMulU()   const { return symbol == MULu;   }
  bool isDivS()   const { return symbol == DIVs;   }
  bool isDivU()   const { return symbol == DIVu;   }
  bool isRemS()   const { return symbol == REMs;   }
  bool isRemU()   const { return symbol == REMu;   }
  bool isModS()   const { return symbol == MODs;   }
  bool isUndef()  const { return symbol == UNDEF;  }

  bool isDff()      const { return (symbol & ~FLGMASK) == DFF;      }
  bool isSDff()     const { return (symbol & ~FLGMASK) == sDFF;     }
  bool isADff()     const { return (symbol & ~FLGMASK) == aDFF;     }
  bool isDffRs()    const { return (symbol & ~FLGMASK) == DFFrs;    }
  bool isDLatch()   const { return (symbol & ~FLGMASK) == DLATCH;   }
  bool isADLatch()  const { return (symbol & ~FLGMASK) == aDLATCH;  }
  bool isDLatchRs() const { return (symbol & ~FLGMASK) == DLATCHrs; }
  bool isLatchRs()  const { return (symbol & ~FLGMASK) == LATCHrs;  }

  bool getClkEdge()  const { return model::getClkEdge(symbol);  }
  bool getEnaLevel() const { return model::getEnaLevel(symbol); }
  bool getRstLevel() const { return model::getRstLevel(symbol); }
  bool getSetLevel() const { return model::getSetLevel(symbol); }
  bool getRstValue() const { return model::getRstValue(symbol); }

  bool isGate() const { return  props.cell &&  props.soft; }
  bool isCell() const { return  props.cell && !props.soft; }
  bool isSoft() const { return !props.cell &&  props.soft; }
  bool isHard() const { return !props.cell && !props.soft; }

  bool isCombinational() const { return props.combinational; }
  bool isConstant()      const { return props.constant;      }
  bool isIdentity()      const { return props.identity;      }
  bool isCommutative()   const { return props.commutative;   }
  bool isAssociative()   const { return props.associative;   }
  bool isRegroupable()   const { return props.regroupable;   }
  bool isNegative()      const { return props.negative;      }

  bool isCmbGate() const { return isGate() &&  isCombinational(); }
  bool isSeqGate() const { return isGate() && !isCombinational(); }

  uint16_t getInNum()  const { return nIn;  }
  uint16_t getOutNum() const { return nOut; }

  /// Checks whether the cell type does not specify the number of inputs.
  bool isInNumFixed() const { return nIn != AnyArity; }
  /// Checks whether the cell type does not specify the number of outputs.
  bool isOutNumFixed() const { return nOut != AnyArity; }

  /// Checks whether the cell type has implementation.
  bool hasImpl() const { return implID != OBJ_NULL_ID; }
  /// Returns the implementation: NetID or SubnetID.
  uint64_t getImpl() { return implID; }

  /// Checks whether the cell type is implemented by Net.
  bool isNet() const { return NetID::checkTag(implID); }
  /// Returns the net ID of the cell type.
  NetID getNetID() const { return implID; }
  /// Returns the net of the cell type.
  const Net &getNet() const;
  /// Sets the net implementation.
  void setNet(NetID netID) { implID = netID; }

  /// Checks whether the cell type is implemented by Subnet.
  bool isSubnet() const { return SubnetID::checkTag(implID); }
  /// Returns the subnet ID of the cell type.
  SubnetID getSubnetID() const { return implID; }
  /// Return the subnet of the cell type.
  const Subnet &getSubnet() const;
  /// Sets the subnet implementation.
  void setSubnet(SubnetID subnetID) { implID = subnetID; }

  /// Checks whether the cell type has attributes.
  bool hasAttr() const { return attrID != OBJ_NULL_ID; }
  /// Returns the cell type attributes.
  const CellTypeAttr &getAttr() const { return CellTypeAttr::get(attrID); }

private:
  CellType(CellSymbol symbol,
           const std::string &name,
           uint64_t implID,
           CellTypeAttrID attrID,
           CellProperties props,
           uint16_t nIn,
           uint16_t nOut):
    nameID(makeString(name)),
    implID(implID),
    attrID(attrID),
    symbol(symbol),
    props(props),
    nIn(nIn),
    nOut(nOut) {}

  const StringID nameID;

  /// Implementation: NetID or SubnetID.
  uint64_t implID;

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

inline std::string getCellTypeName(CellSymbol symbol,
                                   const CellType::PortWidths &widthIn,
                                   const CellType::PortWidths &widthOut) {
  return "cell_type_name"; // FIXME:
}

inline CellTypeID makeCellType(CellSymbol symbol,
                               const std::string &name,
                               uint64_t implID,
                               CellTypeAttrID attrID,
                               CellProperties props,
                               uint16_t nIn,
                               uint16_t nOut) {
  return allocateObject<CellType>(
      symbol, name, implID, attrID, props, nIn, nOut);
}

inline CellTypeID makeCellType(CellSymbol symbol,
                               const std::string &name,
                               CellProperties props,
                               uint16_t nIn,
                               uint16_t nOut) {
  return makeCellType(symbol, name, OBJ_NULL_ID, OBJ_NULL_ID, props, nIn, nOut);
}

inline CellTypeID makeSoftType(CellSymbol symbol,
                               const std::string &name,
                               uint64_t implID,
                               const CellType::PortVector &ports) {
  const auto attrID = makeCellTypeAttr(ports);
  const CellProperties props{0, 1, 0, 0, 0, 0, 0, 0, 0};
  const auto nIn = CellTypeAttr::getInBitWidth(ports);
  const auto nOut = CellTypeAttr::getOutBitWidth(ports);
  return makeCellType(symbol, name, implID, attrID, props, nIn, nOut);
}

inline CellTypeID makeSoftType(CellSymbol symbol,
                               const std::string &name,
                               uint64_t implID,
                               const CellType::PortWidths &widthIn,
                               const CellType::PortWidths &widthOut) {
  const auto attrID = makeCellTypeAttr(widthIn, widthOut);
  const CellProperties props{0, 1, 0, 0, 0, 0, 0, 0, 0};
  const auto nIn  = CellTypeAttr::getBitWidth(widthIn);
  const auto nOut = CellTypeAttr::getBitWidth(widthOut);
  return makeCellType(symbol, name, implID, attrID, props, nIn, nOut);
}

inline CellTypeID makeSoftType(CellSymbol symbol,
                               const std::string &name,
                               uint64_t implID,
                               uint16_t widthArg,
                               uint16_t widthRes) {
  const CellType::PortWidths widthIn{widthArg};
  const CellType::PortWidths widthOut{widthRes};
  return makeSoftType(symbol, name, implID, widthIn, widthOut);
}

inline CellTypeID makeSoftType(CellSymbol symbol,
                               const std::string &name,
                               uint64_t implID,
                               uint16_t widthLhs,
                               uint16_t widthRhs,
                               uint16_t widthRes) {
  const CellType::PortWidths widthIn{widthLhs, widthRhs};
  const CellType::PortWidths widthOut{widthRes};
  return makeSoftType(symbol, name, implID, widthIn, widthOut);
}

inline CellTypeID makeSoftType(CellSymbol symbol,
                               const std::string &name,
                               uint64_t implID,
                               uint16_t widthArg1,
                               uint16_t widthArg2,
                               uint16_t widthArg3,
                               uint16_t widthRes) {
  const CellType::PortWidths widthIn{widthArg1, widthArg2, widthArg3};
  const CellType::PortWidths widthOut{widthRes};
  return makeSoftType(symbol, name, implID, widthIn, widthOut);
}

inline CellTypeID makeHardType(CellSymbol symbol,
                               const std::string &name,
                               const CellType::PortVector &ports) {
  const auto attrID = makeCellTypeAttr(ports);
  const CellProperties props{0, 0, 0, 0, 0, 0, 0, 0, 0};
  const auto nIn = CellTypeAttr::getInBitWidth(ports);
  const auto nOut = CellTypeAttr::getOutBitWidth(ports);
  return makeCellType(symbol, name, OBJ_NULL_ID, attrID, props, nIn, nOut);
}

inline CellTypeID makeHardType(CellSymbol symbol,
                               const std::string &name,
                               const CellType::PortWidths &widthIn,
                               const CellType::PortWidths &widthOut) {
  const auto attrID = makeCellTypeAttr(widthIn, widthOut);
  const CellProperties props{0, 0, 0, 0, 0, 0, 0, 0, 0};
  const auto nIn = CellTypeAttr::getBitWidth(widthIn);
  const auto nOut = CellTypeAttr::getBitWidth(widthOut);
  return makeCellType(symbol, name, OBJ_NULL_ID, attrID, props, nIn, nOut);
}

inline CellTypeID makeHardType(CellSymbol symbol,
                               const std::string &name,
                               uint16_t widthArg,
                               uint16_t widthRes) {
  const CellType::PortWidths widthIn{widthArg};
  const CellType::PortWidths widthOut{widthRes};
  return makeHardType(symbol, name, widthIn, widthOut);
}

inline CellTypeID makeHardType(CellSymbol symbol,
                               const std::string &name,
                               uint16_t widthLhs,
                               uint16_t widthRhs,
                               uint16_t widthRes) {
  const CellType::PortWidths widthIn{widthLhs, widthRhs};
  const CellType::PortWidths widthOut{widthRes};
  return makeHardType(symbol, name, widthIn, widthOut);
}

inline CellTypeID makeHardType(CellSymbol symbol,
                               const std::string &name,
                               uint16_t widthArg1,
                               uint16_t widthArg2,
                               uint16_t widthArg3,
                               uint16_t widthRes) {
  const CellType::PortWidths widthIn{widthArg1, widthArg2, widthArg3};
  const CellType::PortWidths widthOut{widthRes};
  return makeHardType(symbol, name, widthIn, widthOut);
}

//===----------------------------------------------------------------------===//
// Standard Cell Types
//===----------------------------------------------------------------------===//

#define CELL_TYPE_ID(symbol)  CELL_TYPE_ID_##symbol
#define CELL_TYPE_SID(symbol) CELL_TYPE_SID_##symbol

#define DECLARE_CELL_TYPE_ID(symbol) \
  /* Full cell type identifier */ \
  extern const CellTypeID CELL_TYPE_ID(symbol); \
  /* Short cell type identifier */ \
  extern const uint32_t CELL_TYPE_SID(symbol);

#define UTOPIA_FOREACH_GATE(S) DECLARE_CELL_TYPE_ID(S)
#include "celltype_gates.inc"

#define UTOPIA_FOREACH_GATE(S) \
  case S: return CELL_TYPE_ID_##S;
constexpr uint64_t getCellTypeID(CellSymbol symbol) {
  switch(symbol) {
#include "celltype_gates.inc"
  default: return OBJ_NULL_ID;
  }
}

#define UTOPIA_FOREACH_GATE(S) \
  case S: return CELL_TYPE_SID_##S;
constexpr uint32_t getCellTypeSID(CellSymbol symbol) {
  switch(symbol) {
#include "celltype_gates.inc"
  default: return -1u;
  }
}

inline const CellType &getCellType(CellSymbol symbol) {
  return CellType::get(getCellTypeID(symbol));
}

} // namespace eda::gate::model
