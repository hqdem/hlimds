//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"

#include <vector>

namespace eda::gate::model {

/// Topologically sorted combinational subnet.
class Subnet final {
public:
  /// Connection end (driver).
  struct Driver {
    /// Entry index.
    uint32_t entry;
    /// Output port.
    uint32_t port;
  };

  /// Cell entry.
  struct Entry {
    /// Cell type identifier or OBJ_NULL_ID (for undefined cells).
    CellTypeID cellTypeID;
    /// Cell identifier or OBJ_NULL_ID (for unbound cells).
    CellID cellID;
    /// Input connections.
    std::vector<Driver> drivers;
  };

  /// Constructs a subnet.
  Subnet(const std::vector<Entry> &entries):
      entries(entries) {}

private:
  /// Topologically sorted vector of entries.
  std::vector<Entry> entries;
};

} // namespace eda::gate::model
