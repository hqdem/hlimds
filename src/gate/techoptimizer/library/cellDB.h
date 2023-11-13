//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/model2/celltype.h"
#include "gate/techoptimizer/library/subnetattr.h"

using CellTypeID = eda::gate::model::CellTypeID;

namespace eda::gate::tech_optimizer {
  class CellDB final{
  public:
    CellDB(const std::list<CellTypeID> &cellType);

    std::unordered_map<eda::gate::model::SubnetID, Subnetattr> 
        &getSubnetMap() const;

  private:
    std::unordered_map<eda::gate::model::SubnetID, Subnetattr>;
    std::unordered_map<uint64_t, eda::gate::model::SubnetID>;

  }
} // namespace eda::gate::tech_optimizer