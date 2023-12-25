//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/techoptimizer/library/cellDB.h"

using SubnetBuilder = eda::gate::model::SubnetBuilder;
using CellType = eda::gate::model::CellType;

namespace eda::gate::tech_optimizer {

  CellDB::CellDB(const std::list<CellTypeID> &cellTypeIDs) {

    for (const CellTypeID& cellTypeID : cellTypeIDs) {
      CellType cellType = CellType::get(cellTypeID);
      SubnetBuilder subnetBuilder;

      size_t inNum = cellType.getInNum();
      size_t idx[inNum];
      std::vector<eda::gate::model::Subnet::Link> linkList;
      for (size_t i = 0; i < inNum; ++i) {
        idx[i] = subnetBuilder.addCell(model::CellSymbol::IN, SubnetBuilder::INPUT);
        linkList.push_back(idx[i]);
      }

      idx[inNum] = subnetBuilder.addCell(cellTypeID, linkList);

      subnetBuilder.addCell(model::CellSymbol::OUT, 
          eda::gate::model::Subnet::Link(idx[inNum]), SubnetBuilder::OUTPUT);

      SubnetID subnetID = subnetBuilder.make();

      subnets.push_back(subnetID);

      Subnetattr subnetattr;
      subnetToAttr.push_back(std::make_pair(subnetID, subnetattr));

      ttSubnet.push_back(std::make_pair(eda::gate::model::evaluate(
          model::Subnet::get(subnetID)), subnetID));
    }
  }
/*
    void setFFTypeIDs(std::list<CellTypeID> &triggTypeIDs) {
      for (const CellTypeID& cellTypeID : cellTypeIDs) {
      CellType cellType = CellType::get(cellTypeID);
      SubnetBuilder subnetBuilder;

      size_t inNum = cellType.getInNum();

      

      size_t idx[inNum];
      std::vector<eda::gate::model::Subnet::Link> linkList;
      for (size_t i = 0; i < inNum; ++i) {
        idx[i] = subnetBuilder.addCell(model::CellSymbol::IN, SubnetBuilder::INPUT);
        linkList.push_back(idx[i]);
      }

      idx[inNum] = subnetBuilder.addCell(cellTypeID, linkList);

      subnetBuilder.addCell(model::CellSymbol::OUT, 
          eda::gate::model::Subnet::Link(idx[inNum]), SubnetBuilder::OUTPUT);

      SubnetID subnetID = subnetBuilder.make();

      subnets.push_back(subnetID);

      Subnetattr subnetattr;
      subnetMap.insert(std::pair<SubnetID, Subnetattr>
          (subnetID, subnetattr));

    }
    }
    */

   std::vector<std::pair<SubnetID, Subnetattr>> &CellDB::getSubnetsAttr() {
    return subnetToAttr;
   }

   std::vector<std::pair<kitty::dynamic_truth_table, 
        SubnetID>> &CellDB::getTTSubnet(){
    return ttSubnet;
   }

const std::vector<SubnetID> &CellDB::getDFF() const {
  return DFF;
}

const std::vector<SubnetID> &CellDB::getDFFrs() const {
  return DFFrs;
}

const std::vector<SubnetID> &CellDB::getLatch() const {
  return Latch;
}

} // namespace eda::gate::tech_optimizer