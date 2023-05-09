//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gate.h"
#include "gate/model/gnet.h"
#include "gate/optimizer/bgnet.h"

#include <vector>

namespace eda::gate::optimizer {

/**
* \brief GNet with input and output bindings.
* \author <a href="mailto:mrpepelulka@gmail.com">Rustamkhan Ramaldanov</a>
*/
class TruthTable {

public:
  using Gate = eda::gate::model::Gate;
  using GateSymbol = eda::gate::model::GateSymbol;
  using GNet = eda::gate::model::GNet;
  using TruthTableList = std::vector<TruthTable>;

  TruthTable() : _raw(0) { }

  TruthTable(uint64_t raw) {
    _raw = raw;
  }

  uint64_t raw() const {
    return _raw;
  }

  static TruthTable zero() {
    return TruthTable(0);
  }

  static TruthTable one() {
    return TruthTable((uint64_t)(-1));
  }

  bool operator== (TruthTable other) {
    return raw() == other.raw();
  }

  // Bitwise operators overloading
  TruthTable operator&(const TruthTable &other) {
    return TruthTable(raw() & other.raw());
  }

  TruthTable operator|(const TruthTable &other) {
    return TruthTable(raw() | other.raw());
  }

  TruthTable operator^(const TruthTable &other) {
    return TruthTable(raw() ^ other.raw());
  }

  TruthTable operator~() const {
    return TruthTable(~raw());
  }

  operator uint64_t() const {
    return raw();
  }

  // Build truthtable for N'th variable (0 <= N <= 5)
  static TruthTable buildNthVar(int n);

  static TruthTable build(const BoundGNet &bGNet);

private:
  static TruthTable applyGateFunc(const GateSymbol::Value func,
                                  const TruthTableList &inputList);

  uint64_t _raw;
};

} // namespace eda::gate::optimizer

// For backward compatibility
namespace std {
  template <> struct hash<eda::gate::optimizer::TruthTable> {
    size_t operator()(const eda::gate::optimizer::TruthTable &x) const {
      return hash<uint64_t>()(x.raw());
    }
  };
} // namespace std
