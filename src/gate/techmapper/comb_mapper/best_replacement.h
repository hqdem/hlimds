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

#include <climits>
#include <unordered_set>

namespace eda::gate::techmapper {

using EntryIndex = uint64_t;
using SubnetID = eda::gate::model::SubnetID;

struct BestReplacement {
  // Flag default cells
  bool isIN = false;
  bool isOUT = false;
  bool isOne = false;
  bool isZero = false;

  // Best liberty cell
  SubnetID subnetID = 0;
  SubnetID getLibertySubnetID() const {
    assert(subnetID != 0);
    return subnetID;
  }

  // Entry idx for connecting the best cut in the initial subnet
  std::vector<uint64_t> entryIDxs;

  // Entry idx in mapped Subnet
  size_t cellIDInMappedSubnet = ULLONG_MAX;
};
} // namespace eda::gate::techmapper
