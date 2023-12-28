//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model2/celltype.h"
#include "gate/model2/subnet.h"
#include "gate/techoptimizer/library/subnetattr.h"

#include "kitty/print.hpp"

#include <list>
#include <unordered_map>
#include <utility>
#include <vector>

using CellTypeID = eda::gate::model::CellTypeID;
using SubnetID = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

  class CellDB final{
  public:
      CellDB(){};
    CellDB(const std::list<CellTypeID> &cellTypeIDs);

    //CellDB();
    //void setFFTypeIDs(std::list<CellTypeID> &triggTypeIDs);

    std::vector<std::pair<SubnetID, Subnetattr>> &getSubnetsAttr();
    std::vector<std::pair<kitty::dynamic_truth_table, 
        SubnetID>> &getTTSubnet();

    std::vector<SubnetID> getSubnetIDsByTT(const kitty::dynamic_truth_table &tt);
    std::optional<Subnetattr> getSubnetAttrBySubnetID(SubnetID id);

    const std::vector<SubnetID> &getDFF() const;
    const std::vector<SubnetID> &getDFFrs() const;
    const std::vector<SubnetID> &getLatch() const;

  private:
    std::vector<SubnetID> subnets;
    std::vector<SubnetID> DFF;
    std::vector<SubnetID> DFFrs;
    std::vector<SubnetID> Latch;

    std::vector<std::pair<SubnetID, Subnetattr>> subnetToAttr;
    std::vector<std::pair<kitty::dynamic_truth_table, SubnetID>> ttSubnet;

  };
} // namespace eda::gate::tech_optimizer