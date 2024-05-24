//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/decomposer/net_decomposer.h"
#include "gate/model/net.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/subnet_transformer.h"
#include "gate/synthesizer/synthesizer.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace eda::gate::model {

struct Domain final {
  // Clock/reset signal.
  CellID source;

  // Flip-flops of the domain.
  // TODO:

  // Hard/soft blocks of the domain.
  // TODO:

  // Subnets of the domain.
  // TODO:
};

using ClockDomain = Domain;
using ResetDomain = Domain;

class DesignBuilder final {
public:
  using SubnetBuilderPtr = optimizer::SubnetBuilderPtr;

  /// Constructs a design builder from the net (imports the net).
  DesignBuilder(const NetID netID) {
    // Generate the soft block implementations.
    synthesizer::synthSoftBlocks(netID);

    std::vector<SubnetID> subnetIDs;
    NetDecomposer::get().decompose(netID, subnetIDs, mapping);
    setEntries(subnetIDs);
  }

  /// Constructs a design builder from the subnet (imports the subnet).
  DesignBuilder(const SubnetID subnetID) {
    std::vector<SubnetID> subnetIDs;
    NetDecomposer::get().decompose(subnetID, subnetIDs, mapping);
    setEntries(subnetIDs);
  }

  /// Returns the number of subnets in the design.
  size_t getSubnetNum() const {
    return subnets.size();
  }

  /// Makes (if required) an i-th subnet and destroys the builder.
  SubnetID getSubnetID(const size_t i) {
    auto &entry = getEntry(i);
    if (entry.subnetID != OBJ_NULL_ID) {
      return entry.subnetID;
    }

    assert(entry.builder != nullptr);
    setSubnetID(i, entry.builder->make());

    return entry.subnetID;
  }

  /// Replaces the i-th subnet w/ the given one.
  void setSubnetID(const size_t i, const SubnetID subnetID) {
    assert(subnetID != OBJ_NULL_ID);

    // TODO: Check nIn and nOut.

    auto &entry = getEntry(i);
    entry.subnetID = subnetID;
    entry.builder = nullptr;
  }

  /// Makes (if required) an i-th builder and invalidates the subnet.
  SubnetBuilder &getSubnetBuilder(const size_t i) {
    auto &entry = getEntry(i);
    if (entry.builder != nullptr) {
       return *entry.builder;
    }
 
    assert(entry.subnetID != OBJ_NULL_ID);
    entry.builder = std::make_unique<SubnetBuilder>(entry.subnetID);
    entry.subnetID = OBJ_NULL_ID;

    return *entry.builder;
  }

  /// Replaces the i-th subnet builder w/ the given one.
  void setSubnetBuilder(const size_t i, const SubnetBuilderPtr &builder) {
    assert(builder != nullptr);

    auto &entry = getEntry(i);
    entry.subnetID = OBJ_NULL_ID;
    entry.builder = builder;
  }

  /// Saves the current subnet.
  void saveSubnet(const size_t i, const std::string &save) {
    auto &entry = getEntry(i);
    entry.history.emplace_back(save, getSubnetID(i));
  }

  /// Returns the previously saved subnet.
  SubnetID getSubnetID(const size_t i, const std::string &save) const {
    const auto &entry = getEntry(i);

    const auto it = std::find_if(entry.history.begin(), entry.history.end(),
        [&save](const NamedSubnet &namedSubnet) {
          return namedSubnet.first == save;
        });

    if (it != entry.history.end()) {
      return it->second;
    }

    return OBJ_NULL_ID;
  }

  /// Erases the i-th subnet transformation history.
  void commit(const size_t i) {
    auto &entry = getEntry(i);
    entry.history.clear();
  }

  /// Rolls back to the given step in history.
  void rollback(const size_t i, const size_t step) {
    assert(subnets.size() > step);
    auto &entry = getEntry(i);
    entry.subnetID = entry.history[step].second;
    entry.builder = nullptr;
    entry.history.resize(step);
  }

  /// Rolls back to the previously saved subnet.
  void rollback(const size_t i, const std::string &save) {
    auto &entry = getEntry(i);
    for (size_t j = 0; j < entry.history.size(); ++j) {
      if (entry.history[j].first == save) {
        rollback(i, j);
        return;
      }
    }
    assert(false && "Subnet not found");
  }

  /// Rolls a step back.
  void stepback(const size_t i) {
    rollback(i, getEntry(i).history.size() - 1);
  }

  /// Replaces the flip-flop or the latch.
  void replaceCell(const CellID oldCellID, const CellID newCellID,
                   const std::vector<uint16_t> &newInputs,
                   const std::vector<uint16_t> &newOutputs);

  /// Constructs a net (exports the design).
  NetID make() {
    const auto subnetIDs = getSubnetIDs();
    return NetDecomposer::get().compose(subnetIDs, mapping);
  }

private:
  using CellMapping = NetDecomposer::CellMapping;
  using NamedSubnet = std::pair<std::string, SubnetID>;

  struct SubnetEntry {
    SubnetEntry(const SubnetID subnetID):
        history{}, subnetID(subnetID), builder(nullptr) {}

    /// Identifiers of the previous subnets.
    std::vector<NamedSubnet> history;
    /// Current subnet identifier.
    SubnetID subnetID;
    /// Current subnet builder.
    SubnetBuilderPtr builder;
  };

  const SubnetEntry &getEntry(const size_t i) const {
    assert(i < subnets.size());
    return subnets[i];
  }

  SubnetEntry &getEntry(const size_t i) {
    assert(i < subnets.size());
    return subnets[i];
  }

  std::vector<SubnetID> getSubnetIDs() {
    std::vector<SubnetID> subnetIDs(subnets.size());
    for (size_t i = 0; i < subnets.size(); ++i) {
      subnetIDs[i] = getSubnetID(i);
    }
    return subnetIDs;
  }

  void setEntries(const std::vector<SubnetID> &subnetIDs) {
    for (const auto &subnetID : subnetIDs) {
      subnets.emplace_back(subnetID);
    }
  }

  // Clock domains.
  // TODO

  // Reset domains.
  // TODO

  std::vector<SubnetEntry> subnets;
  std::vector<CellMapping> mapping;
};

inline SubnetID makeSubnet(const NetID netID) {
  DesignBuilder builder(netID);
  assert(builder.getSubnetNum() == 1);
  return builder.getSubnetID(0);
}

inline NetID makeNet(const SubnetID subnetID) {
  DesignBuilder builder(subnetID);
  return builder.make();
}

} // namespace eda::gate::model
