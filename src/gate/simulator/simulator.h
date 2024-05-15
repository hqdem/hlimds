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

namespace eda::gate::simulator {

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
    const size_t nIn = subnet.getInNum();
    assert(values.size() == nIn);

    for (size_t i = 0; i < nIn; ++i) {
      setValue(i, values[i]);
    }
  }

  /// Sets the input values (value = bit).
  void setInputs(uint64_t values) {
    const size_t nIn = subnet.getInNum();
    assert(nIn <= (sizeof(values) << 3));

    for (size_t i = 0; i < nIn; ++i) {
      setValue(i, (values >> i) & 1);
    }
  }

  /// Sets the input values (value = bool).
  void setInputs(const std::vector<bool> &values) {
    const size_t nIn = subnet.getInNum();
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
      command.op(command.idx, command.links);
    }
  }

private:
  DataChunk &access(Link link) {
    return state[index(link)];
  }

  const DataChunk &access(Link link) const {
    return state[index(link)];
  }

  DataChunk &access(size_t idx) {
    return state[index(idx)];
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
    Command(Function op, size_t idx, const LinkList &links):
        op(op), idx(idx), links(links) {}

    const Function op;
    const size_t idx;
    const LinkList links;
  };

  /// Returns the cell function.
  Function getFunction(const Cell &cell, size_t idx) const;

  /// Compiled program for the given subnet.
  std::vector<Command> program;

  /// Holds the simulation state (accessed via links).
  DataVector state;
  /// Holds the indices in the simulation state vector.
  std::vector<size_t> pos;

  /// Subnet being simulated.
  const Subnet &subnet;

  //------------------------------------------------------------------------//
  // ZERO
  //------------------------------------------------------------------------//

  const Function opZero = [this](size_t idx, const LinkList &links) {
    access(idx) = 0ull;
  };

  Function getZero(uint16_t arity) const {
    assert(arity == 0);
    return opZero;
  }

  //------------------------------------------------------------------------//
  // ONE
  //------------------------------------------------------------------------//

  const Function opOne = [this](size_t idx, const LinkList &links) {
    access(idx) = -1ull;
  };

  Function getOne(uint16_t arity) const {
    assert(arity == 0);
    return opOne;
  }

  //------------------------------------------------------------------------//
  // BUF
  //------------------------------------------------------------------------//

  const Function opBuf = [this](size_t idx, const LinkList &links) {
    access(idx) = value(links[0]);
  };

  Function getBuf(uint16_t arity) const {
    assert(arity == 1);
    return opBuf;
  }

  //------------------------------------------------------------------------//
  // NOT
  //------------------------------------------------------------------------//

  const Function opNot = [this](size_t idx, const LinkList &links) {
    access(idx) = ~value(links[0]);
  };

  Function getNot(uint16_t arity) const {
    assert(arity == 1);
    return opNot;
  }

  //------------------------------------------------------------------------//
  // AND
  //------------------------------------------------------------------------//

  const Function opAnd2 = [this](size_t idx, const LinkList &links) {
    access(idx) = (value(links[0]) & value(links[1]));
  };

  const Function opAnd3 = [this](size_t idx, const LinkList &links) {
    access(idx) = (value(links[0]) & value(links[1]) & value(links[2]));
  };

  const Function opAndN = [this](size_t idx, const LinkList &links) {
    DataChunk result = -1ull;

    for (size_t i = 0; i + 1 < links.size(); i += 2) {
      result &= (value(links[i]) & value(links[i + 1]));
    }
    if (links.size() & 1) {
      result &= value(links.back());
    }

    access(idx) = result;
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

  const Function opOr2 = [this](size_t idx, const LinkList &links) {
    access(idx) = (value(links[0]) | value(links[1]));
  };

  const Function opOr3 = [this](size_t idx, const LinkList &links) {
    access(idx) = (value(links[0]) | value(links[1]) | value(links[2]));
  };

  const Function opOrN = [this](size_t idx, const LinkList &links) {
    DataChunk result = 0ull;

    for (size_t i = 0; i + 1 < links.size(); i += 2) {
      result |= (value(links[i]) | value(links[i + 1]));
    }
    if (links.size() & 1) {
      result |= value(links.back());
    }

    access(idx) = result;
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

  const Function opXor2 = [this](size_t idx, const LinkList &links) {
    access(idx) = (value(links[0]) ^ value(links[1]));
  };

  const Function opXor3 = [this](size_t idx, const LinkList &links) {
    access(idx) = (value(links[0]) ^ value(links[1]) ^ value(links[2]));
  };

  const Function opXorN = [this](size_t idx, const LinkList &links) {
    DataChunk result = 0ull;

    for (size_t i = 0; i + 1 < links.size(); i += 2) {
      result ^= (value(links[i]) ^ value(links[i + 1]));
    }
    if (links.size() & 1) {
      result ^= value(links.back());
    }

    access(idx) = result;
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

  const Function opNand2 = [this](size_t idx, const LinkList &links) {
    access(idx) = ~(value(links[0]) & value(links[1]));
  };

  const Function opNand3 = [this](size_t idx, const LinkList &links) {
    access(idx) = ~(value(links[0]) & value(links[1]) & value(links[2]));
  };

  const Function opNandN = [this](size_t idx, const LinkList &links) {
    DataChunk result = -1ull;

    for (size_t i = 0; i + 1 < links.size(); i += 2) {
      result &= (value(links[i]) & value(links[i + 1]));
    }
    if (links.size() & 1) {
      result &= value(links.back());
    }

    access(idx) = ~result;
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

  const Function opNor2 = [this](size_t idx, const LinkList &links) {
    access(idx) = ~(value(links[0]) | value(links[1]));
  };

  const Function opNor3 = [this](size_t idx, const LinkList &links) {
    access(idx) = ~(value(links[0]) | value(links[1]) | value(links[2]));
  };

  const Function opNorN = [this](size_t idx, const LinkList &links) {
    DataChunk result = 0ull;

    for (size_t i = 0; i + 1 < links.size(); i += 2) {
      result |= (value(links[i]) | value(links[i + 1]));
    }
    if (links.size() & 1) {
      result |= value(links.back());
    }

    access(idx) = ~result;
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

  const Function opXnor2 = [this](size_t idx, const LinkList &links) {
    access(idx) = ~(value(links[0]) ^ value(links[1]));
  };

  const Function opXnor3 = [this](size_t idx, const LinkList &links) {
    access(idx) = ~(value(links[0]) ^ value(links[1]) ^ value(links[2]));
  };

  const Function opXnorN = [this](size_t idx, const LinkList &links) {
    DataChunk result = 0ull;

    for (size_t i = 0; i + 1 < links.size(); i += 2) {
      result ^= (value(links[i]) ^ value(links[i + 1]));
    }
    if (links.size() & 1) {
      result ^= value(links.back());
    }

    access(idx) = ~result;
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

  const Function opMaj3 = [this](size_t idx, const LinkList &links) {
    const auto x = value(links[0]);
    const auto y = value(links[1]);
    const auto z = value(links[2]);

    access(idx) = ((x & y) | (x & z) | (y & z));
  };

  const Function opMajN = [this](size_t idx, const LinkList &links) {
    const size_t n = links.size();
    const size_t k = (n >> 1);

    DataChunk result = 0u;
    for (size_t bit = 0; bit <= DataChunkBits; ++bit) {
      auto upperBits = value(links[n - 1]) >> bit;
      auto zerosLeft = (upperBits == 0);

      auto weight = (upperBits & 1);
      for (size_t i = 0; i < k; i++) {
        const auto upperBits0 = value(links[(i << 1)|0]) >> bit;
        const auto upperBits1 = value(links[(i << 1)|1]) >> bit;

        zerosLeft &= (upperBits0 == 0) && (upperBits1 == 0);
       
        weight += (upperBits0 & 1);
        weight += (upperBits1 & 1);
      }

      if (zerosLeft) break;
      result |= (weight > k) << bit;
    }

    access(idx) = result;
  };

  Function getMaj(uint16_t arity) const {
    assert(arity >= 1 && (arity & 1));

    switch (arity) {
    case  1: return opBuf;
    case  3: return opMaj3;
    default: return opMajN;
    }
  }

  //------------------------------------------------------------------------//
  // CELL
  //------------------------------------------------------------------------//

  const Function opCell = [this](size_t idx, const LinkList &links) {
    const auto &entries = subnet.getEntries();
    const auto &cell = entries[idx].cell;
    const auto &type = cell.getType();
    const auto &subnet = type.getSubnet();

    Simulator simulator(subnet);

    for (size_t i = 0; i < subnet.getInNum(); ++i) {
      simulator.setValue(i, value(links[i]));
    }

    simulator.simulate();

    for (size_t i = 0; i < subnet.getOutNum(); ++i) {
      access(Link(idx, i)) = simulator.value(subnet.getOut(i));
    }
  };

  Function getCell(size_t idx, uint16_t nIn, uint16_t nOut) const {
    const auto &entries = subnet.getEntries();
    assert(idx < entries.size());

    const auto &cell = entries[idx].cell;
    const auto &type = cell.getType();
    assert(type.isSubnet());

    const auto &subnet = type.getSubnet();
    assert(subnet.getInNum() == nIn);
    assert(subnet.getOutNum() == nOut);

    return opCell;
  }
};

} // namespace eda::gate::simulator
