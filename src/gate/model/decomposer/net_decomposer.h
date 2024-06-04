//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/net.h"
#include "gate/model/subnet.h"
#include "util/singleton.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace eda::gate::model {

/// Implements Net <-> {Subnet} decomposition/composition.
class NetDecomposer final : public util::Singleton<NetDecomposer> {
  friend class util::Singleton<NetDecomposer>;

public:
  /// Maps net links to subnet input/output cell indices.
  using LinkMap = std::unordered_map<Link, size_t>;
  /// Maps net cells to the subnet links corresponding to the output ports.
  using CellMap = std::unordered_map<CellID, Subnet::LinkList>;

  /// Maps net cells/links to subnet cell indices.
  struct CellMapping final {
    size_t size;     // Original subnet size.
    LinkMap inputs;  // Decomposition => composition.
    CellMap inners;  // Temporal result of decomposition.
    LinkMap outputs; // Decomposition => composition.
  };

  /// Decomposes the net into subnets and fills the cell mapping.
  void decompose(const NetID netID,
                 std::vector<SubnetID> &subnets,
                 std::vector<CellMapping> &mapping) const;

  /// Imitates decomposition of the net consisting of a single subnet.
  void decompose(const SubnetID subnetID,
                 std::vector<SubnetID> &subnet,
                 std::vector<CellMapping> &mapping) const;

  /// Composes the subnets into a net.
  NetID compose(const std::vector<SubnetID> &subnets,
                const std::vector<CellMapping> &mapping) const;

private:
  NetDecomposer() {}
};

} // namespace eda::gate::model
