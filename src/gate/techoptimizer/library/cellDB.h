//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#include "gate/model2/celltype.h"
#include "gate/model2/subnet.h"
#include "gate/techoptimizer/library/subnetattr.h"

#include "kitty/print.hpp"

#include <list>
#include <map>
#include <unordered_map>

using CellTypeID = eda::gate::model::CellTypeID;
using SubnetID = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

  class CellDB final{
  public:
    CellDB(const std::list<CellTypeID> &cellTypeIDs);

    CellDB();
    //void setFFTypeIDs(std::list<CellTypeID> &triggTypeIDs);

    std::map<SubnetID, Subnetattr> &getSubnetMap();

  private:
    std::vector<SubnetID> subnets;

    std::vector<SubnetID> triggers;

    std::map<SubnetID, Subnetattr> subnetMap;
    std::map< kitty::dynamic_truth_table, 
        eda::gate::model::SubnetID> ttSubnetMap;

  };
} // namespace eda::gate::tech_optimizer