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
#include <utility>
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

class Design final {
public:
  /// Constructs a design from the net (imports the net).
  Design(const std::string &name, const NetID netID): name(name) {
    NetDecomposer::get().decompose(netID, subnets, mapping);
  }

  /// Constructs a trivial design from the subnet (imports the subnet).
  Design(const std::string &name, const SubnetID subnetID): name(name) {
    NetDecomposer::get().decompose(subnetID, subnets, mapping);
  }

  /// Returns the name of the design.
  const std::string &getName() const { return name; }

  /// Constructs a net from the design (exports the design).
  NetID make() const {
    return NetDecomposer::get().compose(subnets, mapping);
  }

  /// Returns the design subnets.
  const std::vector<SubnetID> &getSubnetIDs() const {
    return subnets;
  }

  /// Returns the design subnet.
  SubnetID getSubnetID() const {
    assert(subnets.size() == 1);
    return subnets.front();
  }

  /// Replaces the subnet.
  void replaceSubnet(const size_t i, const SubnetID newSubnetID);

  /// Replaces the flip-flop or the latch.
  void replaceCell(const CellID oldCellID, const CellID newCellID,
                   const std::vector<uint16_t> &newInputs,
                   const std::vector<uint16_t> &newOutputs);

private:
  using CellMapping = NetDecomposer::CellMapping;

  const std::string name;

  // Clock domains.
  // TODO

  // Reset domains.
  // TODO

  std::vector<SubnetID> subnets;
  std::vector<CellMapping> mapping;
};

using DesignID = std::shared_ptr<Design>;

inline SubnetID makeSubnet(NetID netID) {
  Design design("design", netID);
  return design.getSubnetID();
}

inline NetID makeNet(SubnetID subnetID) {
  Design design("design", subnetID);
  return design.make();
}

//===----------------------------------------------------------------------===//
// Design Builder
//===----------------------------------------------------------------------===//

class DesignBuilder final {
public:
  using SubnetBuilderPtr = optimizer::SubnetBuilderPtr;

  DesignBuilder(const std::shared_ptr<Design> &design): design(design) {
    const auto &subnetIDs = design->getSubnetIDs();
    for (const auto &subnetID : subnetIDs) {
      subnets.emplace_back(subnetID);
    }
  }

  DesignBuilder(const std::string &name, const NetID netID) {
    synthesizer::synthSoftBlocks(netID);
    design = std::make_shared<Design>(name, netID);

    const auto &subnetIDs = design->getSubnetIDs(); // TODO: Copy-paste
    for (const auto &subnetID : subnetIDs) {
      subnets.emplace_back(subnetID);
    }
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

  /// Makes a design.
  std::shared_ptr<Design> make() {
    assert(design != nullptr);
    for (size_t i = 0; i < subnets.size(); ++i) {
      design->replaceSubnet(i, getSubnetID(i));
    }
    return design;
  }

private:
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

  std::shared_ptr<Design> design;
  std::vector<SubnetEntry> subnets;
};

} // namespace eda::gate::model
