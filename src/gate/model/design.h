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
#include <tuple>
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
private:
  /// Delete buffers when making subnets.
  static constexpr bool DeleteBuffers = true;
  /// Default design name.
  static constexpr auto DefaultName = "design";

public:
  using SubnetBuilderPtr = optimizer::SubnetBuilderPtr;

  /// Constructs a design builder w/ the given name from the net.
  DesignBuilder(const std::string &name, const NetID netID): name(name) {
    const auto &net = Net::get(netID);

    // Initialize the numbers of inputs and outputs.
    nIn = net.getInNum();
    nOut = net.getOutNum();

    // Generate the soft block implementations.
    synthesizer::synthSoftBlocks(netID);

    std::vector<SubnetID> subnetIDs;
    NetDecomposer::get().decompose(netID, subnetIDs, mapping);
    setEntries(subnetIDs);
  }

  /// Constructs a design builder from the net.
  DesignBuilder(const NetID netID):
      DesignBuilder(DefaultName, netID) {}

  /// Constructs a design builder w/ the given name from the subnet.
  DesignBuilder(const std::string &name, const SubnetID subnetID): name(name) {
    const auto &subnet = Subnet::get(subnetID);

    // Initialize the numbers of inputs and outputs.
    nIn = subnet.getInNum();
    nOut = subnet.getOutNum();

    std::vector<SubnetID> subnetIDs;
    NetDecomposer::get().decompose(subnetID, subnetIDs, mapping);
    setEntries(subnetIDs);
  }

  /// Constructs a design builder from the subnet.
  DesignBuilder(const SubnetID subnetID):
      DesignBuilder(DefaultName, subnetID) {}

  /// Return the design name.
  const std::string getName() const {
    return name;
  }

  /// Sets the design name.
  void setName(const std::string &name) {
    this->name = name;
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
    setSubnetID(i, entry.builder->make(DeleteBuffers));

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
  const SubnetBuilderPtr &getSubnetBuilder(const size_t i) {
    auto &entry = getEntry(i);
    if (entry.builder != nullptr) {
       return entry.builder;
    }
 
    assert(entry.subnetID != OBJ_NULL_ID);
    entry.builder = std::make_unique<SubnetBuilder>(entry.subnetID);
    entry.subnetID = OBJ_NULL_ID;

    return entry.builder;
  }

  /// Replaces the i-th subnet builder w/ the given one.
  void setSubnetBuilder(const size_t i, const SubnetBuilderPtr &builder) {
    auto &entry = getEntry(i);
    entry.subnetID = OBJ_NULL_ID;
    entry.builder = builder;
  }

  /// Returns the number of input/output/internal cells of the i-th subnet.
  std::tuple<size_t, size_t, size_t> getCellNum(const size_t i) const {
    size_t n, m, k;
    const auto &entry = getEntry(i);
    if (entry.subnetID != OBJ_NULL_ID) {
      const auto &subnet = Subnet::get(entry.subnetID);
      n = subnet.getInNum();
      m = subnet.getOutNum();
      k = subnet.getCellNum();
    } else {
      assert(entry.builder != nullptr);
      const auto &builder = entry.builder;
      n = builder->getInNum();
      m = builder->getOutNum();
      k = builder->getCellNum();
    }

    return std::make_tuple(n, m, k - n - m);
  }

  /// Returns the number of input/output/internal cells of the design.
  std::tuple<size_t, size_t, size_t> getCellNum() const {
    size_t nInt{0};
    for (size_t i = 0; i < subnets.size(); ++i) {
      nInt += std::get<2>(getCellNum(i));
    }
    return std::make_tuple(nIn, nOut, nInt);
  }

  /// Makes the subnets.
  void makeSubnets() {
    for (size_t i = 0; i < subnets.size(); ++i) {
      auto &entry = getEntry(i);
      if (entry.subnetID == OBJ_NULL_ID) {
        assert(entry.builder != nullptr);
        setSubnetID(i, entry.builder->make(DeleteBuffers));
      }
    }
  }

  /// Returns the global check points.
  std::vector<std::string> getPoints() const {
    return points;
  }

  /// Checks if there is a global check point w/ the given name.
  bool hasPoint(const std::string &point) const {
    return std::find(points.begin(), points.end(), point) != points.end();
  }

  /// Makes a check point for the i-th subnet.
  void save(const size_t i, const std::string &point) {
    auto &entry = getEntry(i);
    entry.points.emplace(point, getSubnetID(i));
  }

  /// Makes a global check point.
  void save(const std::string &point) {
    for (size_t i = 0; i < subnets.size(); ++i) {
      save(i, point);
    }

    if (!hasPoint(point)) {
      points.push_back(point);
    }
  }

  /// Rolls back to the given check point of the i-th subnet.
  void rollback(const size_t i, const std::string &point) {
    auto &entry = getEntry(i);
    entry.subnetID = getSubnetID(i, point);
    entry.builder = nullptr;
  }

  /// Rolls back to the global check point.
  void rollback(const std::string &point) {
    for (size_t i = 0; i < subnets.size(); ++i) {
      rollback(i, point);
    }
  }

  /// Returns the subnet from the given check point.
  SubnetID getSubnetID(const size_t i, const std::string &point) const {
    const auto &entry = getEntry(i);

    const auto it = entry.points.find(point);
    assert(it != entry.points.end());

    return it->second;
  }

  /// Replaces the flip-flop or the latch.
  void replaceCell(const CellID oldCellID, const CellID newCellID,
                   const std::vector<uint16_t> &newInputs,
                   const std::vector<uint16_t> &newOutputs);

  /// Checks if the design is tech-mapped.
  bool isTechMapped() {
    // It is assumed that either all subnets are tech-mapped
    // or all subnets are not tech-mapped.
    if (subnets.empty()) {
      return false;
    }

    const auto subnetID = getSubnetID(0);
    const auto &subnet = Subnet::get(subnetID);

    return subnet.isTechMapped();
  }

  /// Constructs a net.
  NetID make() {
    const auto subnetIDs = getSubnetIDs();
    return NetDecomposer::get().compose(subnetIDs, mapping);
  }

  /// Constructs a net for the given check point.
  NetID make(const std::string &point) {
    const auto subnetIDs = getSubnetIDs(point);
    return NetDecomposer::get().compose(subnetIDs, mapping);
  }

private:
  using CellMapping = NetDecomposer::CellMapping;

  struct SubnetEntry {
    SubnetEntry(const SubnetID subnetID): subnetID(subnetID) {}

    /// Check points.
    std::unordered_map<std::string, SubnetID> points;
    /// Current subnet identifier.
    SubnetID subnetID;
    /// Current subnet builder.
    SubnetBuilderPtr builder{nullptr};
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

  std::vector<SubnetID> getSubnetIDs(const std::string &point) {
    std::vector<SubnetID> subnetIDs(subnets.size());
    for (size_t i = 0; i < subnets.size(); ++i) {
      subnetIDs[i] = getSubnetID(i, point);
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

  // Design name.
  std::string name;

  std::vector<std::string> points;
  std::vector<SubnetEntry> subnets;
  std::vector<CellMapping> mapping;

  size_t nIn;  // FIXME:
  size_t nOut; // FIXME:
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
