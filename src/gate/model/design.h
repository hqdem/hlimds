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
#include "gate/synthesizer/synthesizer.h"

#include <cassert>
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
using EntryID = model::EntryID;

class DesignBuilder final {
public:
  /// Index of virtual primary inputs subnet entry.
  static constexpr size_t PISubnetEntryIdx = static_cast<size_t>(-1);
  /// Index of virtual primary outputs subnet entry.
  static constexpr size_t POSubnetEntryIdx = static_cast<size_t>(-2);

private:
  /// Delete buffers when making subnets.
  static constexpr bool DeleteBuffers = true;
  /// Default design name.
  static constexpr auto DefaultName = "Design";

public:
  using SubnetBuilderPtr = std::shared_ptr<SubnetBuilder>;
  using ConnectionDesc = NetDecomposer::ConnectionDesc;
  using ArcToDesc = std::unordered_map<size_t, ConnectionDesc>;
  using LinkToDesc = NetDecomposer::LinkToDesc;
  using SubnetToSubnetSet = std::vector<std::unordered_set<size_t>>;
  using SubnetToArcDescs = std::vector<ArcToDesc>;
  using SubnetToFFSet = std::vector<std::unordered_set<EntryID>>;

  struct SubnetEntry {
    SubnetEntry(
        const SubnetID subnetID,
        const std::unordered_set<size_t> &arcs,
        const std::unordered_map<size_t, ConnectionDesc> &arcToDesc) {

      this->subnetID = subnetID;
      for (const auto &arcSubnet : arcs) {
        if (arcSubnet == PISubnetEntryIdx) {
          connectedPI = true;
          PIArcDesc = arcToDesc.find(arcSubnet)->second;
          continue;
        }
        if (arcSubnet == POSubnetEntryIdx) {
          connectedPO = true;
          POArcDesc = arcToDesc.find(arcSubnet)->second;
          continue;
        }
        this->arcs.insert(arcSubnet);
        this->arcToDesc[arcSubnet] = arcToDesc.find(arcSubnet)->second;
      }
    };

    /**
     * @brief Returns true if the currect entry has an input arc with the i-th
     * entry index, false otherwise.
     */
    bool hasArc(const size_t i) const {
      return arcs.find(i) != arcs.end();
    }

    /**
     * @brief Returns true if the current entry has an input arc with the
     * primary inputs of the net, false otherwise.
     */
    bool hasPIArc() const {
      return connectedPI;
    }

    /**
     * @brief Returns true if the current entry has an input arc with the
     * primary outputs of the net, false otherwise.
     */
    bool hasPOArc() const {
      return connectedPO;
    }

    /// Returns all fanin arcs (no PI and PO arcs) of the current subnet.
    const std::unordered_set<size_t> &getInArcs() const {
      return arcs;
    }

    /// Returns i-th entry arc descriptor.
    const ConnectionDesc &getArcDesc(const size_t i) const {
      const auto it = arcToDesc.find(i);
      assert(it != arcToDesc.end() && "No such arc");
      return it->second;
    }
    /// Returns primary inputs arc descriptor.
    const ConnectionDesc &getPIArcDesc() const {
      assert(hasPIArc());
      return PIArcDesc;
    }
    /// Returns primary outputs arc descriptor.
    const ConnectionDesc &getPOArcDesc() const {
      assert(hasPOArc());
      return POArcDesc;
    }

    /// Check points.
    std::unordered_map<std::string, SubnetID> points;
    /// Current subnet identifier.
    SubnetID subnetID;
    /// Current subnet builder.
    SubnetBuilderPtr builder{nullptr};
    /// Adjency list of connected input subnets.
    std::unordered_set<size_t> arcs;
    /// Arcs descriptors.
    std::unordered_map<size_t, ConnectionDesc> arcToDesc;
    /// Net's primary inputs connection flag.
    bool connectedPI{false};
    /// Net's primary inputs arc descriptor.
    ConnectionDesc PIArcDesc;
    /// Net's primary ouputs connection flag.
    bool connectedPO{false};
    /// Net's primary outputs arc descriptor.
    ConnectionDesc POArcDesc;
  };

private:
  void initialize(const NetID netID) {
    assert(netID != OBJ_NULL_ID);
    const auto &net = Net::get(netID);

    nIn = net.getInNum();
    nOut = net.getOutNum();

    // Generate the soft block implementations.
    synthesizer::synthSoftBlocks(netID);
    // Decompose the net into subnets.
    NetDecomposer::get().decompose(netID, result);

    SubnetToSubnetSet adjList;
    SubnetToArcDescs arcDescs;
    setAdjList(result, adjList, arcDescs);
    setEntries(result, adjList, arcDescs);
  }

  void initialize(const SubnetID subnetID) {
    assert(subnetID != OBJ_NULL_ID);
    const auto &subnet = Subnet::get(subnetID);

    nIn = subnet.getInNum();
    nOut = subnet.getOutNum();

    NetDecomposer::get().decompose(subnetID, result);
    SubnetToSubnetSet adjList;
    SubnetToArcDescs arcDescs;
    setAdjList(result, adjList, arcDescs);
    setEntries(result, adjList, arcDescs);
  }

  std::pair<bool, EntryID> getPIConnectionEntry(const size_t i) const {
    const auto &subnets = result.subnets;
    for (const auto &[oldLink, oldIdx] : subnets[i].mapping.inputs) {
      const auto oldSourceID = oldLink.source.getCellID();
      if (Cell::get(oldSourceID).getTypeID() == CELL_TYPE_ID_IN) {
        return { true, oldIdx };
      }
    }
    return { false, EntryID() };
  }

  std::pair<bool, EntryID> getPOConnectionEntry(const size_t i) const {
    const auto &subnets = result.subnets;
    for (const auto &[oldLink, oldIdx] : subnets[i].mapping.outputs) {
      const auto oldTargetID = oldLink.target.getCellID();
      if (Cell::get(oldTargetID).getTypeID() == CELL_TYPE_ID_OUT) {
        return { true, oldIdx };
      }
    }
    return { false, EntryID() };
  }

public:
  /// Constructs a design builder w/ the given name from the net.
  DesignBuilder(const std::string &name, const NetID netID):
      name(name), typeID(OBJ_NULL_ID) {
    initialize(netID);
  }

  /// Constructs a design builder from the net.
  DesignBuilder(const NetID netID):
      DesignBuilder(DefaultName, netID) {}

  /// Constructs a design builder w/ the given name from the subnet.
  DesignBuilder(const std::string &name, const SubnetID subnetID):
        name(name), typeID(OBJ_NULL_ID) {
    initialize(subnetID);
  }

  /// Constructs a design builder from the subnet.
  DesignBuilder(const SubnetID subnetID):
      DesignBuilder(DefaultName, subnetID) {}

  /// Constructs a design builder w/ the given name from the cell type.
  DesignBuilder(const std::string &name, const CellTypeID typeID):
      name(name), typeID(typeID) {
    assert(typeID != OBJ_NULL_ID);

    const auto &type = CellType::get(typeID);
    assert(type.hasImpl());

    if (type.isNet()) {
      initialize(type.getNetID());
    } else {
      initialize(type.getSubnetID());
    }
  }

  /// Constructs a design builder from the cell type.
  DesignBuilder(const CellTypeID typeID):
      DesignBuilder(DefaultName, typeID) {}

  /// Return the design name.
  const std::string getName() const {
    return name;
  }

  /// Sets the design name.
  void setName(const std::string &name) {
    this->name = name;
  }

  /// Checks whether the design has type information.
  bool hasType() const {
    return typeID != OBJ_NULL_ID;
  }

  /// Returns the type identifier associated w/ the design.
  CellTypeID getTypeID() const {
    return typeID;
  }

  /// Returns the type information associated w/ the design.
  CellType &getType() const {
    assert(typeID != OBJ_NULL_ID);
    return CellType::get(typeID);
  }

  /// Returns the number of subnets in the design.
  size_t getSubnetNum() const {
    return entries.size();
  }

  /// Returns the i-th subnet entry.
  const SubnetEntry &getEntry(const size_t i) const {
    assert(i < entries.size());
    return entries[i];
  }

  /// Returns the i-th subnet entry.
  SubnetEntry &getEntry(const size_t i) {
    assert(i < entries.size());
    return entries[i];
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

  /// Returns the number of primary inputs.
  size_t getInNum() const { return nIn; }

  /// Returns the number of primary outputs.
  size_t getOutNum() const { return nOut; }

  /// Returns the number of input/output/internal cells of the i-th subnet.
  std::tuple<EntryID, EntryID, EntryID> getCellNum(const size_t i,
                                                   const bool withBufs) const;

  /// Returns the number of input/output/internal cells of the design.
  std::tuple<size_t, size_t, size_t> getCellNum(const bool withBufs) const {
    size_t nInt{0};
    for (size_t i = 0; i < entries.size(); ++i) {
      nInt += std::get<2>(getCellNum(i, withBufs));
    }
    return std::make_tuple(nIn, nOut, nInt);
  }

  /// Makes the subnets.
  void makeSubnets() {
    for (size_t i = 0; i < entries.size(); ++i) {
      auto &entry = getEntry(i);
      if (entry.builder != nullptr) {
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
    for (size_t i = 0; i < entries.size(); ++i) {
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
    for (size_t i = 0; i < entries.size(); ++i) {
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
    if (entries.empty()) {
      return false;
    }

    const auto subnetID = getSubnetID(0);
    const auto &subnet = Subnet::get(subnetID);

    return subnet.isTechMapped();
  }

  /// Constructs a net.
  NetID make() {
    updateSubnets();
    return NetDecomposer::get().compose(result);
  }

  /// Constructs a net for the given check point.
  NetID make(const std::string &point) {
    updateSubnets(point);
    return NetDecomposer::get().compose(result);
  }

private:
  using CellMapping = NetDecomposer::CellMapping;

  void updateSubnets() {
    for (size_t i = 0; i < entries.size(); ++i) {
      result.subnets[i].subnetID = getSubnetID(i);
    }
  }

  void updateSubnets(const std::string &point) {
    for (size_t i = 0; i < entries.size(); ++i) {
      result.subnets[i].subnetID = getSubnetID(i, point);
    }
  }

  void setEntries(
      const NetDecomposer::Result &result,
      const SubnetToSubnetSet &adjList,
      const SubnetToArcDescs &arcDescs) {

    const auto &subnets = result.subnets;
    for (size_t i = 0; i < subnets.size(); ++i) {
      entries.emplace_back(subnets[i].subnetID, adjList[i], arcDescs[i]);
    }
  }

  SubnetToFFSet findFlipFlopPIs(
      const NetDecomposer::Result &result) const;

  SubnetToSubnetSet findArcs(
      const NetDecomposer::Result &result,
      const SubnetToFFSet &flipFlopPOs,
      SubnetToArcDescs &arcDesc) const;

  void setAdjList(
      const NetDecomposer::Result &result,
      SubnetToSubnetSet &adjList,
      SubnetToArcDescs &arcDescs) const;

  // Clock domains.
  // TODO

  // Reset domains.
  // TODO

  /// Design name.
  std::string name;
  /// Type information.
  const CellTypeID typeID{OBJ_NULL_ID};

  std::vector<std::string> points;
  std::vector<SubnetEntry> entries;

  NetDecomposer::Result result;

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
