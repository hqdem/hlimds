
#pragma once

#include "gate/model2/cell.h"
#include "gate/model2/subnet.h"

#include <unordered_map>

namespace eda::gate::tech_optimizer {
/**
 * \brief Struct for descriptions Super Gate
 * \author <a href="mailto:dgaryaev@ispras.ru"></a>
 */

  using CellID = uint64_t;
  using SubnetID = model::SubnetID;

  struct BestReplacement {

    CellID cellID;
    SubnetID subnetID;

    // matching technology cells CellID to circuts CellID
    std::unordered_map<CellID, CellID>;

  };
} // namespace eda::gate::tech_optimizer
