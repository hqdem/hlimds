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
#include "gate/techmapper/library/subnetattr.h"

#include "kitty/print.hpp"

//#include <list>
#include <unordered_map>
#include <utility>
#include <vector>

using CellTypeID = eda::gate::model::CellTypeID;
using SubnetID = eda::gate::model::SubnetID;

namespace eda::gate::tech_optimizer {

  class CellDB final{
  public:
    CellDB(const std::vector<CellTypeID> &cellTypeIDs,
           const std::vector<CellTypeID> &cellTypeFFIDs,
           const std::vector<CellTypeID> &cellTypeFFrsIDs,
           const std::vector<CellTypeID> &cellTypeLatchIDs);

    //CellDB();
    //void setFFTypeIDs(std::vectorCellTypeID> &triggTypeIDs);

    std::vector<std::pair<SubnetID, Subnetattr>> &getSubnetsAttr();
    std::vector<std::pair<kitty::dynamic_truth_table, 
        SubnetID>> &getTTSubnet();

    std::vector<SubnetID> getPatterns();

    std::vector<SubnetID> getSubnetIDsByTT(const kitty::dynamic_truth_table &tt) const;
    const Subnetattr &getSubnetAttrBySubnetID(const SubnetID id) const;

    const std::vector<std::pair<SubnetID, Subnetattr>> &getDFF() const;
    const std::vector<std::pair<SubnetID, Subnetattr>> &getDFFrs() const;
    const std::vector<std::pair<SubnetID, Subnetattr>> &getLatch() const;

  private:
    std::vector<SubnetID> subnets;
    //std::vector<SubnetID> DFF;
    //std::vector<SubnetID> DFFrs;
    //std::vector<SubnetID> Latch;
    std::vector<std::pair<SubnetID, Subnetattr>> DFF;
    std::vector<std::pair<SubnetID, Subnetattr>> DFFrs;
    std::vector<std::pair<SubnetID, Subnetattr>> Latch;

    std::vector<std::pair<SubnetID, Subnetattr>> subnetToAttr;
    std::vector<std::pair<kitty::dynamic_truth_table, SubnetID>> ttSubnet;

  };
} // namespace eda::gate::tech_optimizer
