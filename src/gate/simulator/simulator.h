//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnetview.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <vector>

namespace eda::gate::simulator {

/**
 * @brief Subnet simulator.
 */
class Simulator final {
public:
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetBuilderPtr = std::shared_ptr<SubnetBuilder>;
  using SubnetView = model::SubnetView;
  using EntryID = model::EntryID;
  using Cell = model::Subnet::Cell;
  using Link = model::Subnet::Link;
  using LinkList = model::Subnet::LinkList;

  using DataChunk = uint64_t;
  using DataVector = std::vector<DataChunk>;

  /// Data chunk size in bits.
  static constexpr uint16_t DataChunkBits = (sizeof(DataChunk) << 3);

  Simulator(const SubnetBuilderPtr &builder);

  /// Evaluates the output and inner values from the input ones.
  template <typename T = DataVector>
  void simulate(const T &values) { 
    setInputs(values);
    simulate();
  }

  /// Sets the input values (value = uint64_t).
  void setInputs(const DataVector &values) {
    const uint16_t nIn = subnet.getInNum();
    assert(values.size() == nIn);

    for (uint16_t i = 0; i < nIn; ++i) {
      setInput(i, values[i]);
    }
  }

  /// Sets the input values (value = bit).
  void setInputs(uint64_t values) {
    const uint16_t nIn = subnet.getInNum();
    assert(nIn <= (sizeof(values) << 3));

    for (uint16_t i = 0; i < nIn; ++i) {
      setInput(i, (values >> i) & 1);
    }
  }

  /// Sets the input values (value = bool).
  void setInputs(const std::vector<bool> &values) {
    const uint16_t nIn = subnet.getInNum();
    assert(values.size() == nIn);

    for (uint16_t i = 0; i < nIn; ++i) {
      setInput(i, values[i]);
    }
  }

  /// Sets the input value.
  template <typename T = DataChunk>
  void setInput(uint16_t i, T value) {
    const auto entryID = subnet.getIn(i).idx;
    setValue(entryID, value);
  }

  /// Gets the output value.
  template <typename T = DataChunk>
  T getOutput(uint16_t i) const {
    const auto entryID = subnet.getOut(i).idx;
    return getValue(entryID);
  }

  /// Gets the output link value.
  template <typename T = DataChunk>
  T getValue(Link link) const {
    return static_cast<T>(value(link));
  }

  /// Gets the cell value.
  template <typename T = DataChunk>
  T getValue(EntryID entryID) const {
    return getValue(Link(entryID));
  }

  /// Sets the cell value.
  template <typename T = DataChunk>
  void setValue(EntryID entryID, T value) {
    access(entryID) = static_cast<DataChunk>(value);
  }

  /// Executes the compiled program.
  void simulate() {
    for (const auto &command : program) {
      command.op(command.entryID, command.links);
    }
  }

private:
  DataChunk &access(Link link) {
    return state[index(link)];
  }

  const DataChunk &access(Link link) const {
    return state[index(link)];
  }

  DataChunk &access(EntryID entryID) {
    return state[index(entryID)];
  }

  const DataChunk &access(EntryID entryID) const {
    return state[index(entryID)];
  }

  DataChunk value(Link link) const {
    return link.inv ? ~access(link) : access(link);
  }

  size_t index(Link link) const {
    return index(link.idx) + link.out;
  }

  size_t index(EntryID entryID) const {
    const auto idx = subnet.getParent().builder().getDataVal<size_t>(entryID);
    return pos[idx];
  }

  /// Cell function.
  using Function = std::function<void(EntryID, const LinkList&)>;

  /// Single command.
  struct Command final {
    Command(Function op, EntryID entryID, const LinkList &links):
        op(op), entryID(entryID), links(links) {}

    const Function op;
    const EntryID entryID;
    const LinkList links;
  };

  /// Returns the cell function.
  Function getFunction(const Cell &cell, EntryID entryID) const;

  /// Compiled program for the given subnet.
  std::vector<Command> program;

  /// Holds the simulation state (accessed via links).
  DataVector state;
  /// Holds the indices in the simulation state vector.
  std::vector<size_t> pos;

  const SubnetView subnet;

  //------------------------------------------------------------------------//
  // ZERO
  //------------------------------------------------------------------------//

  const Function opZero = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = 0ull;
  };

  Function getZero(uint16_t arity) const {
    assert(arity == 0);
    return opZero;
  }

  //------------------------------------------------------------------------//
  // ONE
  //------------------------------------------------------------------------//

  const Function opOne = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = -1ull;
  };

  Function getOne(uint16_t arity) const {
    assert(arity == 0);
    return opOne;
  }

  //------------------------------------------------------------------------//
  // BUF
  //------------------------------------------------------------------------//

  const Function opBuf = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = value(links[0]);
  };

  Function getBuf(uint16_t arity) const {
    assert(arity == 1);
    return opBuf;
  }

  //------------------------------------------------------------------------//
  // NOT
  //------------------------------------------------------------------------//

  const Function opNot = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = ~value(links[0]);
  };

  Function getNot(uint16_t arity) const {
    assert(arity == 1);
    return opNot;
  }

  //------------------------------------------------------------------------//
  // AND
  //------------------------------------------------------------------------//

  const Function opAnd2 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = (value(links[0]) & value(links[1]));
  };

  const Function opAnd3 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = (value(links[0]) & value(links[1]) & value(links[2]));
  };

  const Function opAndN = [this](EntryID entryID, const LinkList &links) {
    DataChunk result = -1ull;

    const uint32_t nLinks = links.size();
    for (uint32_t i = 0; i + 1 < nLinks; i += 2) {
      result &= (value(links[i]) & value(links[i + 1]));
    }
    if (nLinks & 1) {
      result &= value(links.back());
    }

    access(entryID) = result;
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

  const Function opOr2 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = (value(links[0]) | value(links[1]));
  };

  const Function opOr3 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = (value(links[0]) | value(links[1]) | value(links[2]));
  };

  const Function opOrN = [this](EntryID entryID, const LinkList &links) {
    DataChunk result = 0ull;

    const uint32_t nLinks = links.size();
    for (uint32_t i = 0; i + 1 < links.size(); i += 2) {
      result |= (value(links[i]) | value(links[i + 1]));
    }
    if (nLinks & 1) {
      result |= value(links.back());
    }

    access(entryID) = result;
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

  const Function opXor2 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = (value(links[0]) ^ value(links[1]));
  };

  const Function opXor3 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = (value(links[0]) ^ value(links[1]) ^ value(links[2]));
  };

  const Function opXorN = [this](EntryID entryID, const LinkList &links) {
    DataChunk result = 0ull;

    const uint32_t nLinks = links.size();
    for (uint32_t i = 0; i + 1 < nLinks; i += 2) {
      result ^= (value(links[i]) ^ value(links[i + 1]));
    }
    if (nLinks & 1) {
      result ^= value(links.back());
    }

    access(entryID) = result;
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

  const Function opNand2 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = ~(value(links[0]) & value(links[1]));
  };

  const Function opNand3 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = ~(value(links[0]) & value(links[1]) & value(links[2]));
  };

  const Function opNandN = [this](EntryID entryID, const LinkList &links) {
    DataChunk result = -1ull;

    const uint32_t nLinks = links.size();
    for (uint32_t i = 0; i + 1 < nLinks; i += 2) {
      result &= (value(links[i]) & value(links[i + 1]));
    }
    if (nLinks & 1) {
      result &= value(links.back());
    }

    access(entryID) = ~result;
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

  const Function opNor2 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = ~(value(links[0]) | value(links[1]));
  };

  const Function opNor3 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = ~(value(links[0]) | value(links[1]) | value(links[2]));
  };

  const Function opNorN = [this](EntryID entryID, const LinkList &links) {
    DataChunk result = 0ull;

    const uint32_t nLinks = links.size();
    for (uint32_t i = 0; i + 1 < nLinks; i += 2) {
      result |= (value(links[i]) | value(links[i + 1]));
    }
    if (nLinks & 1) {
      result |= value(links.back());
    }

    access(entryID) = ~result;
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

  const Function opXnor2 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = ~(value(links[0]) ^ value(links[1]));
  };

  const Function opXnor3 = [this](EntryID entryID, const LinkList &links) {
    access(entryID) = ~(value(links[0]) ^ value(links[1]) ^ value(links[2]));
  };

  const Function opXnorN = [this](EntryID entryID, const LinkList &links) {
    DataChunk result = 0ull;

    const uint32_t nLinks = links.size();
    for (uint32_t i = 0; i + 1 < nLinks; i += 2) {
      result ^= (value(links[i]) ^ value(links[i + 1]));
    }
    if (nLinks & 1) {
      result ^= value(links.back());
    }

    access(entryID) = ~result;
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

  const Function opMaj3 = [this](EntryID entryID, const LinkList &links) {
    const auto x = value(links[0]);
    const auto y = value(links[1]);
    const auto z = value(links[2]);

    access(entryID) = ((x & y) | (x & z) | (y & z));
  };

  const Function opMajN = [this](EntryID entryID, const LinkList &links) {
    const auto n = links.size();
    const auto k = (n >> 1);

    DataChunk result = 0u;
    for (uint16_t bit = 0; bit <= DataChunkBits; ++bit) {
      auto upperBits = value(links[n - 1]) >> bit;
      auto zerosLeft = (upperBits == 0);

      auto weight = (upperBits & 1);
      for (uint16_t i = 0; i < k; i++) {
        const auto upperBits0 = value(links[(i << 1)|0]) >> bit;
        const auto upperBits1 = value(links[(i << 1)|1]) >> bit;

        zerosLeft &= (upperBits0 == 0) && (upperBits1 == 0);
       
        weight += (upperBits0 & 1);
        weight += (upperBits1 & 1);
      }

      if (zerosLeft) break;
      result |= (weight > k) << bit;
    }

    access(entryID) = result;
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

  const Function opCell = [this](EntryID entryID, const LinkList &links) {
    const auto &cell = subnet.getParent().builder().getCell(entryID);
    const auto &type = cell.getType();
    const auto &subnet = type.getSubnet();

    auto builder = std::make_shared<SubnetBuilder>(subnet);
    Simulator simulator(builder);

    for (uint16_t i = 0; i < subnet.getInNum(); ++i) {
      simulator.setValue(i, value(links[i]));
    }

    simulator.simulate();

    for (uint16_t i = 0; i < subnet.getOutNum(); ++i) {
      access(Link(entryID, i)) = simulator.value(subnet.getOut(i));
    }
  };

  Function getCell(EntryID entryID, uint16_t nIn, uint16_t nOut) const {
    const auto &cell = subnet.getParent().builder().getCell(entryID);
    const auto &type = cell.getType();
    assert(type.isSubnet());

    const auto &subnet = type.getSubnet();
    assert(subnet.getInNum() == nIn);
    assert(subnet.getOutNum() == nOut);

    return opCell;
  }
};

} // namespace eda::gate::simulator
