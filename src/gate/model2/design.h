//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/decomposer/net_decomposer.h"

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
  explicit Design(const NetID netID) {
    NetDecomposer::get().decompose(netID, subnets, mapping);
  }

  /// Constructs a net from the design (exports the design).
  NetID make() const {
    return NetDecomposer::get().compose(subnets, mapping);
  }

  /// Returns the design subnets.
  const std::vector<SubnetID> &getSubnets() const {
    return subnets;
  }

  /// Replaces the subnet.
  void replaceSubnet(const size_t i, const SubnetID newSubnetID);

  /// Replaces the flip-flop or the latch.
  void replaceCell(const CellID oldCellID, const CellID newCellID,
                   const std::vector<uint16_t> &newInputs,
                   const std::vector<uint16_t> &newOutputs);

private:
  using CellMapping = NetDecomposer::CellMapping;

  // Clock domains.
  // TODO

  // Reset domains.
  // TODO

  std::vector<SubnetID> subnets;
  std::vector<CellMapping> mapping;
};

} // namespace eda::gate::model
