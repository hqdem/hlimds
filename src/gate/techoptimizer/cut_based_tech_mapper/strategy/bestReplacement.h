
#pragma once

#include "gate/model2/cell.h"
#include "gate/model2/subnet.h"

#include <unordered_map>

namespace eda::gate::tech_optimizer {
/**
 * \brief Struct for descriptions Super Gate
 * \author <a href="mailto:dgaryaev@ispras.ru"></a>
 */

  using EntryIndex = uint64_t;
  using SubnetID = eda::gate::model::SubnetID;

  struct BestReplacement {

    EntryIndex cellID;

    // Sunbnet with custom cell from CellDB
    SubnetID subnetID;

    // matching technology cells inputs CellID with circuts CellID
    std::unordered_map<EntryIndex, EntryIndex> matchMap;

  };
} // namespace eda::gate::tech_optimizer
