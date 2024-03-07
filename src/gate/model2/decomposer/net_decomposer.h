//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/net.h"
#include "gate/model2/subnet.h"
#include "util/singleton.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace eda::gate::model {

/// Implements Net <-> {Subnet} decomposition/composition.
class NetDecomposer final : public util::Singleton<NetDecomposer> {
  friend class util::Singleton<NetDecomposer>;

public:
  /// Maps net links to subnet cell indices.
  using LinkMap = std::unordered_map<Link, size_t>;
  /// Maps net cells to subnet cell indices w/ inversion flags.
  using CellMap = std::unordered_map<CellID, std::pair<size_t, bool>>;

  /// Maps net cells/links to subnet cell indices.
  struct CellMapping final {
    size_t size;
    LinkMap inputs;
    CellMap inners;
    LinkMap outputs;
  };

  /// Decomposes the net into subnets and fills the cell mapping.
  std::vector<SubnetID> decompose(NetID netID,
                                  std::vector<CellMapping> &mapping) const;

  /// Composes the subnets into a net.
  NetID compose(const std::vector<SubnetID> &subnets,
                const std::vector<CellMapping> &mapping) const;

private:
  NetDecomposer() {}
};

} // namespace eda::gate::model
