
#pragma once

#include "gate/model2/cell.h"
#include "gate/model2/subnet.h"

#include <limits.h>
#include <unordered_set>

namespace eda::gate::tech_optimizer {
/**
 * \brief Struct for descriptions Super Gate
 * \author <a href="mailto:dgaryaev@ispras.ru"></a>
 */

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
    std::unordered_set<uint64_t> entryIDxs;

    // Entry idx in mapped Subnet
    size_t cellIDInMappedSubnet = ULLONG_MAX;
  };
} // namespace eda::gate::tech_optimizer