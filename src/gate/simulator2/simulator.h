//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <vector>

namespace eda::gate::simulator2 {

/**
 * \brief Subnet simulator.
 */
class Simulator final {
public:
  using Subnet = eda::gate::model::Subnet;
  using Cell = Subnet::Cell;
  using Link = Subnet::Link;
  using LinkList = Subnet::LinkList;

  using DataChunk = uint64_t;
  using DataVector = std::vector<DataChunk>;

  /// Data chunk size in bits.
  static constexpr size_t DataChunkBits = (sizeof(DataChunk) << 3);

  Simulator(const Subnet &subnet);

  /// Evaluates the output and inner values from the input ones.
  template <typename T = DataVector>
  void simulate(const T &values) { 
    setInputs(values);
    simulate();
  }

  /// Sets the input values (value = uint64_t).
  void setInputs(const DataVector &values) {
    assert(values.size() == nIn);
    for (size_t i = 0; i < nIn; ++i) {
      setValue(i, values[i]);
    }
  }

  /// Sets the input values (value = bit).
  void setInputs(uint64_t values) {
    assert(nIn <= (sizeof(values) << 3));
    for (size_t i = 0; i < nIn; ++i) {
      setValue(i, (values >> i) & 1);
    }
  }

  /// Sets the input values (value = bool).
  void setInputs(const std::vector<bool> &values) {
    assert(values.size() == nIn);
    for (size_t i = 0; i < nIn; ++i) {
      setValue(i, values[i]);
    }
  }

  /// Gets the output link value.
  template <typename T = DataChunk>
  T getValue(Link link) const {
    return static_cast<T>(value(link));
  }

  /// Gets the cell value.
  template <typename T = DataChunk>
  T getValue(size_t idx) const {
    return getValue(Link(idx));
  }

  /// Sets the cell value.
  template <typename T = DataChunk>
  void setValue(size_t idx, T value) {
    access(idx) = static_cast<DataChunk>(value);
  }

  /// Executes the compiled program.
  void simulate() {
    for (const auto &command : program) {
      command.op(command.out, command.in);
    }
  }

private:
  DataChunk &access(Link link) {
    return state[index(link)];
  }

  DataChunk &access(size_t idx) {
    return state[index(idx)];
  }

  const DataChunk &access(Link link) const {
    return state[index(link)];
  }

  const DataChunk &access(size_t idx) const {
    return state[index(idx)];
  }

  DataChunk value(Link link) const {
    return link.inv ? ~access(link) : access(link);
  }

  size_t index(Link link) const {
    return pos[link.idx] + link.out;
  }

  size_t index(size_t idx) const {
    return pos[idx];
  }

  /// Cell function.
  using Function = std::function<void(size_t, const LinkList&)>;

  /// Single command.
  struct Command final {
    Command(Function op, size_t out, const LinkList &in):
        op(op), out(out), in(in) {}

    const Function op;
    const size_t out;
    const LinkList in;
  };

  /// Returns the cell function.
  Function getFunction(const Cell &cell) const;

  /// Compiled program for the given subnet.
  std::vector<Command> program;

  /// Holds the simulation state (accessed via links).
  DataVector state;
  /// Holds the indices in the simulation state vector.
  std::vector<size_t> pos;

  /// Number of inputs.
  const size_t nIn;
  /// Number of outputs.
  const size_t nOut;

  //------------------------------------------------------------------------//
  // ZERO
  //------------------------------------------------------------------------//

  const Function opZero = [this](size_t out, const LinkList &in) {
    access(out) = 0ull;
  };

  Function getZero(uint16_t arity) const {
    assert(arity == 0);
    return opZero;
  }

  //------------------------------------------------------------------------//
  // ONE
  //------------------------------------------------------------------------//

  const Function opOne = [this](size_t out, const LinkList &in) {
    access(out) = -1ull;
  };

  Function getOne(uint16_t arity) const {
    assert(arity == 0);
    return opOne;
  }

  //------------------------------------------------------------------------//
  // BUF
  //------------------------------------------------------------------------//

  const Function opBuf = [this](size_t out, const LinkList &in) {
    access(out) = value(in[0]);
  };

  Function getBuf(uint16_t arity) const {
    assert(arity == 1);
    return opBuf;
  }

  //------------------------------------------------------------------------//
  // NOT
  //------------------------------------------------------------------------//

  const Function opNot = [this](size_t out, const LinkList &in) {
    access(out) = ~value(in[0]);
  };

  Function getNot(uint16_t arity) const {
    assert(arity == 1);
    return opNot;
  }

  //------------------------------------------------------------------------//
  // AND
  //------------------------------------------------------------------------//

  const Function opAnd2 = [this](size_t out, const LinkList &in) {
    access(out) = (value(in[0]) & value(in[1]));
  };

  const Function opAnd3 = [this](size_t out, const LinkList &in) {
    access(out) = (value(in[0]) & value(in[1]) & value(in[2]));
  };

  const Function opAndN = [this](size_t out, const LinkList &in) {
    DataChunk result = -1ull;

    for (size_t i = 0; i + 1 < in.size(); i += 2) {
      result &= (value(in[i]) & value(in[i + 1]));
    }
    if (in.size() & 1) {
      result &= value(in.back());
    }

    access(out) = result;
  };

  Function getAnd(uint16_t arity) const {
    assert(arity >= 1);

    switch (arity) {
    case  1: return opBuf;
    case  2: return opAnd2;
    case  3: return opAnd3;
    default: return opAndN;
    }
  }

  //------------------------------------------------------------------------//
  // OR
  //------------------------------------------------------------------------//

  const Function opOr2 = [this](size_t out, const LinkList &in) {
    access(out) = (value(in[0]) | value(in[1]));
  };

  const Function opOr3 = [this](size_t out, const LinkList &in) {
    access(out) = (value(in[0]) | value(in[1]) | value(in[2]));
  };

  const Function opOrN = [this](size_t out, const LinkList &in) {
    DataChunk result = 0ull;

    for (size_t i = 0; i + 1 < in.size(); i += 2) {
      result |= (value(in[i]) | value(in[i + 1]));
    }
    if (in.size() & 1) {
      result |= value(in.back());
    }

    access(out) = result;
  };

  Function getOr(uint16_t arity) const {
    assert(arity >= 1);

    switch (arity) {
    case  1: return opBuf;
    case  2: return opOr2;
    case  3: return opOr3;
    default: return opOrN;
    }
  }

  //------------------------------------------------------------------------//
  // XOR
  //------------------------------------------------------------------------//

  const Function opXor2 = [this](size_t out, const LinkList &in) {
    access(out) = (value(in[0]) ^ value(in[1]));
  };

  const Function opXor3 = [this](size_t out, const LinkList &in) {
    access(out) = (value(in[0]) ^ value(in[1]) ^ value(in[2]));
  };

  const Function opXorN = [this](size_t out, const LinkList &in) {
    DataChunk result = 0ull;

    for (size_t i = 0; i + 1 < in.size(); i += 2) {
      result ^= (value(in[i]) ^ value(in[i + 1]));
    }
    if (in.size() & 1) {
      result ^= value(in.back());
    }

    access(out) = result;
  };

  Function getXor(uint16_t arity) const {
    assert(arity >= 1);

    switch (arity) {
    case  1: return opBuf;
    case  2: return opXor2;
    case  3: return opXor3;
    default: return opXorN;
    }
  }

  //------------------------------------------------------------------------//
  // NAND
  //------------------------------------------------------------------------//

  const Function opNand2 = [this](size_t out, const LinkList &in) {
    access(out) = ~(value(in[0]) & value(in[1]));
  };

  const Function opNand3 = [this](size_t out, const LinkList &in) {
    access(out) = ~(value(in[0]) & value(in[1]) & value(in[2]));
  };

  const Function opNandN = [this](size_t out, const LinkList &in) {
    DataChunk result = -1ull;

    for (size_t i = 0; i + 1 < in.size(); i += 2) {
      result &= (value(in[i]) & value(in[i + 1]));
    }
    if (in.size() & 1) {
      result &= value(in.back());
    }

    access(out) = ~result;
  };

  Function getNand(uint16_t arity) const {
    assert(arity >= 1);

    switch (arity) {
    case  1: return opNot;
    case  2: return opNand2;
    case  3: return opNand3;
    default: return opNandN;
    }
  }

  //------------------------------------------------------------------------//
  // NOR
  //------------------------------------------------------------------------//

  const Function opNor2 = [this](size_t out, const LinkList &in) {
    access(out) = ~(value(in[0]) | value(in[1]));
  };

  const Function opNor3 = [this](size_t out, const LinkList &in) {
    access(out) = ~(value(in[0]) | value(in[1]) | value(in[2]));
  };

  const Function opNorN = [this](size_t out, const LinkList &in) {
    DataChunk result = 0ull;

    for (size_t i = 0; i + 1 < in.size(); i += 2) {
      result |= (value(in[i]) | value(in[i + 1]));
    }
    if (in.size() & 1) {
      result |= value(in.back());
    }

    access(out) = ~result;
  };

  Function getNor(uint16_t arity) const {
    assert(arity >= 1);

    switch (arity) {
    case  1: return opNot;
    case  2: return opNor2;
    case  3: return opNor3;
    default: return opNorN;
    }
  }

  //------------------------------------------------------------------------//
  // XNOR
  //------------------------------------------------------------------------//

  const Function opXnor2 = [this](size_t out, const LinkList &in) {
    access(out) = ~(value(in[0]) ^ value(in[1]));
  };

  const Function opXnor3 = [this](size_t out, const LinkList &in) {
    access(out) = ~(value(in[0]) ^ value(in[1]) ^ value(in[2]));
  };

  const Function opXnorN = [this](size_t out, const LinkList &in) {
    DataChunk result = 0ull;

    for (size_t i = 0; i + 1 < in.size(); i += 2) {
      result ^= (value(in[i]) ^ value(in[i + 1]));
    }
    if (in.size() & 1) {
      result ^= value(in.back());
    }

    access(out) = ~result;
  };

  Function getXnor(uint16_t arity) const {
    assert(arity >= 1);

    switch (arity) {
    case  1: return opNot;
    case  2: return opXnor2;
    case  3: return opXnor3;
    default: return opXnorN;
    }
  }

  //------------------------------------------------------------------------//
  // MAJ
  //------------------------------------------------------------------------//

  const Function opMaj3 = [this](size_t out, const LinkList &in) {
    const auto x = value(in[0]);
    const auto y = value(in[1]);
    const auto z = value(in[2]);

    access(out) = ((x & y) | (x & z) | (y & z));
  };

  const Function opMajN = [this](size_t out, const LinkList &in) {
    const size_t n = in.size();
    const size_t k = (n >> 1);

    DataChunk result = 0u;
    for (size_t bit = 0; bit <= DataChunkBits; ++bit) {
      auto upperBits = value(in[n - 1]) >> bit;
      auto zerosLeft = (upperBits == 0);

      auto weight = (upperBits & 1);
      for (size_t i = 0; i < k; i++) {
        const auto upperBits0 = value(in[(i << 1)|0]) >> bit;
        const auto upperBits1 = value(in[(i << 1)|1]) >> bit;

        zerosLeft &= (upperBits0 == 0) && (upperBits1 == 0);
       
        weight += (upperBits0 & 1);
        weight += (upperBits1 & 1);
      }

      if (zerosLeft) break;
      result |= (weight > k) << bit;
    }

    access(out) = result;
  };

  Function getMaj(uint16_t arity) const {
    assert(arity >= 1 && (arity & 1));

    switch (arity) {
    case  1: return opBuf;
    case  3: return opMaj3;
    default: return opMajN;
    }
  }
};

} // namespace eda::gate::simulator2
