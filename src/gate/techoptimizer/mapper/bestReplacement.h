
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
    bool isIN = false;
    bool isOUT = false;
    bool isOne = false;
    bool isZero = false;

    size_t cellIDInMappedSubnet = ULLONG_MAX;

    // Sunbnet with custom cell from CellDB
    SubnetID subnetID = 0;
    SubnetID getLibertySubnetID() const {
      assert(subnetID != 0);
      return subnetID;
    }

    // matching technology cells inputs CellID with circuits CellID
    //std::unordered_map<EntryIndex, EntryIndex> matchMap;

    // Entry indices in the mapped circuit
    std::unordered_set<uint64_t> entryIDxs;
  };
} // namespace eda::gate::tech_optimizer