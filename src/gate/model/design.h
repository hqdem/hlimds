//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/decomposer/net_decomposer.h"
#include "gate/model/net.h"
#include "gate/model/subnet.h"

#include <cassert>
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
  const std::vector<SubnetID> &getSubnets() const {
    return subnets;
  }

  /// Returns the design subnet.
  SubnetID getSubnet() const {
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

inline SubnetID makeSubnet(NetID netID) {
  Design design("design", netID);
  return design.getSubnet();
}

inline NetID makeNet(SubnetID subnetID) {
  Design design("design", subnetID);
  return design.make();
}

} // namespace eda::gate::model
