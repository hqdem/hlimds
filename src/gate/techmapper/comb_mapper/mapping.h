//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/cell.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/cut_extractor.h"

#include <climits>
#include <unordered_map>
#include <vector>

namespace eda::gate::techmapper {

using EntryIndex = uint64_t;
using SubnetID = model::SubnetID;

struct MappingItem {
  // Cell type
  enum Type {DEFAULT, IN, OUT, ONE, ZERO};

  inline Type getType() const { return type; }
  inline void setType(Type type) { this->type = type; }

  MappingItem(Type type = DEFAULT) : type (type) {};

private:
  // Flags for special cells.
  Type type = DEFAULT;

  // Selected Liberty cell.
  SubnetID subnetID = 0;

public:
  // Returns SubnetID of the selected Liberty cell.
  SubnetID getSubnetID() const {
    assert(subnetID != 0);
    return subnetID;
  }

  void setSubnetID(SubnetID subnetID) {
    this->subnetID = subnetID;
  }

  // The way the selected cut used to be connected in this initial Subnet.
  std::vector<uint64_t> inputs;

  // Entry idx in mapped Subnet.
  size_t cellID = ULLONG_MAX;
};

using Mapping = std::unordered_map<EntryIndex, MappingItem>;

} // namespace eda::gate::techmapper
