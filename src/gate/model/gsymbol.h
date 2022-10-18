//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <iostream>

namespace eda::gate::model {

/**
 * \brief Defines names of supported logical gates and flip-flops/latches.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class GateSymbol final {
public:
  enum Value : uint16_t {
    //----------------------------------------------------------------------------
    // Logic gates
    //----------------------------------------------------------------------------

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
    /// Peirce's arrow: OUT <= ~(X | Y (| ...)).
    NOR,
    /// Exclusive NOR: OUT <= ~(X + Y (+ ...) (mod 2)).
    XNOR,

    //----------------------------------------------------------------------------
    // Flip-flops and latches
    //----------------------------------------------------------------------------

    /// D latch (Q, D, ENA):
    /// Q(t) = ENA(level1) ? D : Q(t-1).
    LATCH,
    /// D flip-flop (Q, D, CLK):
    /// Q(t) = CLK(posedge) ? D : Q(t-1).
    DFF,
    /// D flip-flop w/ (asynchronous) reset and set (Q, D, CLK, RST, SET):
    /// Q(t) = RST(level1) ? 0 : (SET(level1) ? 1 : (CLK(posedge) ? D : Q(t-1))).
    DFFrs
  };

  GateSymbol() = default;
  constexpr GateSymbol(Value value): _value(value) {}

  explicit operator bool() const = delete;        
  constexpr operator Value() const { return _value; }

  bool isConstant() const {
    return _value == ZERO || _value == ONE;
  }

  bool isIdentity() const {
    return _value == NOP;
  }

  bool isCommutative() const {
    return _value != LATCH && _value != DFF && _value != DFFrs;
  }

  bool isAssociative() const {
    return _value == AND || _value == OR || _value == XOR;
  }

private:
  Value _value;
};

std::ostream& operator <<(std::ostream &out, GateSymbol gate);

} // namespace eda::gate::model
