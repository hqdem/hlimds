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
  /// Type of incoming signal.
  enum SignalType {
    DATA,
    CLOCK,
    RESET,
    SET,
    ENABLE,
  };

  struct ConnectionDesc final {
    SignalType signalType;
  };

  /// Maps net links to subnet input/output cell indices.
  using LinkMap = std::unordered_map<Link, EntryID>;
  /// Maps net cells to the subnet links corresponding to the output ports.
  using CellMap = std::unordered_map<CellID, Subnet::LinkList>;
  /// Maps subnet entry index to connection descriptor.
  using EntryToDesc = std::unordered_map<EntryID, ConnectionDesc>;

  using LinkDescVec = std::vector<std::pair<Link, ConnectionDesc>>;
  using LinkToDesc = std::unordered_map<Link, ConnectionDesc>;

  /// Maps net cells/links to subnet cell indices.
  struct CellMapping final {
    size_t size;     // Original subnet size.
    LinkMap inputs;  // Result => composition.
    CellMap inners;  // Temporal result of decomposition.
    LinkMap outputs; // Result => composition.
  };

  /// Subnet information.
  struct SubnetDesc final {
    SubnetDesc(const SubnetID subnetID,
               const CellMapping &mapping,
               const EntryToDesc &entryToDesc):
      subnetID(subnetID), mapping(mapping), entryToDesc(entryToDesc) {}

    SubnetID subnetID;
    CellMapping mapping;
    EntryToDesc entryToDesc;
  };

  /// Result of decomposition.
  struct Result final {
    List<CellID> inputs;
    List<CellID> outputs;
    std::vector<SubnetDesc> subnets;
  };

  /// Decomposes the net into subnets.
  bool decompose(const NetID netID, Result &decomposition) const;

  /// Imitates decomposition of the net consisting of a single subnet.
  bool decompose(const SubnetID subnetID, Result &decomposition) const;

  /// Composes the subnets into a net.
  NetID compose(const Result &decomposition) const;

private:
  NetDecomposer() {}
};

} // namespace eda::gate::model
