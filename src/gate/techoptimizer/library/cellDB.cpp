//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/techoptimizer/library/cellDB.h"

#include <algorithm>
#include <random>

using CellSymbol = eda::gate::model::CellSymbol;
using CellType = eda::gate::model::CellType;
using Link = eda::gate::model::Subnet::Link;
using SubnetBuilder = eda::gate::model::SubnetBuilder;

namespace eda::gate::tech_optimizer {

CellDB::CellDB(const std::vector<CellTypeID> &cellTypeIDs,
               const std::vector<CellTypeID> &cellTypeFFIDs,
               const std::vector<CellTypeID> &cellTypeFFrsIDs,
               const std::vector<CellTypeID> &cellTypeLatchIDs) {
  std::cout << "Count of liberty CellType = " << cellTypeIDs.size() << std::endl;
  /*int count = 0;
  for (const CellTypeID &cellTypeID : cellTypeIDs) {
    CellType &cellType = CellType::get(cellTypeID);

    std::vector<int> permutationVec(cellType.getInNum());
    std::iota(permutationVec.begin(), permutationVec.end(), 0);
    do {
      count++;
      SubnetBuilder subnetBuilder;
      uint16_t linkArray[cellType.getInNum()];
      for (size_t i = 0; i < cellType.getInNum(); ++i) {
        auto inputIdx = subnetBuilder.addInput();
        linkArray[permutationVec.at(i)] = inputIdx.idx;
      }
      std::vector<Link> linkList;
      for (size_t i = 0; i < cellType.getInNum(); ++i) {
        linkList.emplace_back(linkArray[i]);
      }

      auto cellIdx = subnetBuilder.addCell(cellTypeID, linkList);
      subnetBuilder.addOutput(cellIdx);
      SubnetID subnetID = subnetBuilder.make();

      subnets.push_back(subnetID);
      Subnetattr subnetattr{"LibertyCell", cellType.getAttr().area};
      subnetToAttr.emplace_back(subnetID, subnetattr);
      ttSubnet.emplace_back(model::evaluate(cellType.getSubnet()), subnetID);
    } while (std::next_permutation(permutationVec.begin(), permutationVec.end()));
  }
  std::cout << "Count of liberty Subnet = " << count << std::endl;*/

  std::cout << "Count of liberty CellType = " << cellTypeIDs.size() << std::endl;
  int count = 0;
  for (const CellTypeID &cellTypeID : cellTypeIDs) {
    count++;
    CellType &cellType = CellType::get(cellTypeID);
    SubnetBuilder subnetBuilder;
    std::vector<Link> linkList;

    for (size_t i = 0; i < cellType.getInNum(); ++i) {
      linkList.emplace_back(subnetBuilder.addInput());
    }

    auto cellIdx = subnetBuilder.addCell(cellTypeID, linkList);

    subnetBuilder.addOutput(cellIdx);

    SubnetID subnetID = subnetBuilder.make();

    subnets.push_back(subnetID);

    Subnetattr subnetattr{"LibraryCell", cellType.getAttr().area};
    subnetToAttr.emplace_back(subnetID, subnetattr);

    ttSubnet.emplace_back(model::evaluate(
        cellType.getSubnet()).at(0), subnetID);
  }

  for (const CellTypeID &cellTypeID : cellTypeFFIDs) {
    CellType &cellType = CellType::get(cellTypeID);

    SubnetBuilder subnetBuilder;
    std::vector<Link> linkList;

    auto in1 = subnetBuilder.addInput();
    auto in2 = subnetBuilder.addInput();
    linkList.emplace_back(in2);
    linkList.emplace_back(in1);
    auto dff = subnetBuilder.addCell(cellTypeID, linkList);
    subnetBuilder.addOutput(dff);

    SubnetID subnetID = subnetBuilder.make();

    Subnetattr subnetattr{"FF", cellType.getAttr().area};
    DFF.emplace_back(subnetID, subnetattr);
  }
  for (const CellTypeID &cellTypeID : cellTypeFFrsIDs) {
    CellType &cellType = CellType::get(cellTypeID);

    SubnetBuilder subnetBuilder;
    std::vector<Link> linkList;

    auto in1 = subnetBuilder.addInput();
    auto in2 = subnetBuilder.addInput();
    auto in3 = subnetBuilder.addInput();
    auto in4 = subnetBuilder.addInput();

    linkList.emplace_back(in2);
    linkList.emplace_back(in1);
    linkList.emplace_back(in3);
    linkList.emplace_back(in4);

    auto dffrs = subnetBuilder.addCell(cellTypeID, linkList);
    subnetBuilder.addOutput(dffrs);

    SubnetID subnetID = subnetBuilder.make();

    Subnetattr subnetattr{"FFrs", cellType.getAttr().area};
    DFFrs.emplace_back(subnetID, subnetattr);
  }
  for (const CellTypeID &cellTypeID : cellTypeLatchIDs) {
    CellType &cellType = CellType::get(cellTypeID);

    SubnetBuilder subnetBuilder;
    std::vector<Link> linkList;

    auto in1 = subnetBuilder.addInput();
    auto in2 = subnetBuilder.addInput();
    linkList.emplace_back(in1);
    linkList.emplace_back(in2);
    auto latch = subnetBuilder.addCell(cellTypeID, linkList);
    subnetBuilder.addOutput(latch);

    SubnetID subnetID = subnetBuilder.make();

    Subnetattr subnetattr{"Latch", cellType.getAttr().area};
    Latch.emplace_back(subnetID, subnetattr);
  }
  std::cout << "Count of liberty Subnet = " << count << std::endl;
}

std::vector<SubnetID> CellDB::getSubnetIDsByTT(const kitty::dynamic_truth_table& tt) const {
  std::vector<SubnetID> ids;
  for (const auto& pair : ttSubnet) {
    if (pair.first == tt) {
      ids.push_back(pair.second);
    }
  }
  return ids;
}

const Subnetattr &CellDB::getSubnetAttrBySubnetID(const SubnetID id) const {
  for (const auto& pair : subnetToAttr) {
    if (pair.first == id) {
      return pair.second;
    }
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

const std::vector<std::pair<SubnetID, Subnetattr>> &CellDB::getDFF() const {
  return DFF;
}
const std::vector<std::pair<SubnetID, Subnetattr>> &CellDB::getDFFrs() const {
  return DFFrs;
}
const std::vector<std::pair<SubnetID, Subnetattr>> &CellDB::getLatch() const {
  return Latch;
}

} // namespace eda::gate::tech_optimizer
