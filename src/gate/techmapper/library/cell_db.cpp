//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/utils/subnet_truth_table.h"
#include "gate/techmapper/library/cell_db.h"
#include "gate/techmapper/library/liberty_manager.h"

#include <vector>

namespace eda::gate::techmapper {

using CellSymbol = eda::gate::model::CellSymbol;
using CellType = eda::gate::model::CellType;
using Link = eda::gate::model::Subnet::Link;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SubnetID = eda::gate::model::SubnetID;

CellDB::CellDB(const std::vector<CellTypeID> &combCellTypeIDs,
               const SC::StandardSeqMap &seqCellTypeIDs) {

  std::cout << "The number of extracted from Liberty CellTypes is " <<
      combCellTypeIDs.size() << std::endl; // TODO
  int count = 0;
  for (const CellTypeID &cellTypeID : combCellTypeIDs) {
    CellType &cellType = CellType::get(cellTypeID);

    bool needPower = true;
    std::vector<Power> powers;
    if (needPower) {
      std::string cell_name = cellType.getName();

      const auto *cell = LibertyManager::get().getLibrary().getCell(cell_name);

      for (const auto &pin: (*cell).getPins()) {
        if (pin.getIntegerAttribute("direction", 10) & (1 << 1)) {
          for (const auto &inPwr: pin.getInternalPowerGroups()) {
            Power power;

            int i = 0;
            for (const auto &lut: inPwr.getLuts()) {
              float sum = 0;
              for (const auto &it: lut.getValues()) {
                sum += it;
              }
              float med = sum / lut.getValuesSize();
              (i == 0) ? power.fall_power = med : power.rise_power = med;
              i++;
            }
            powers.push_back(power);
          }
        }
      }
    }

    std::vector<int> permutationVec(cellType.getInNum());
    std::iota(permutationVec.begin(), permutationVec.end(), 0);
    do {
      count++;

      SubnetBuilder subnetBuilder;
      SubnetBuilder subnetBuilderForTT;

      std::vector<uint16_t> linkArray(cellType.getInNum());
      std::vector<uint16_t> linkArrayForTT(cellType.getInNum());

      for (size_t i = 0; i < cellType.getInNum(); ++i) {
        auto inputIdx = subnetBuilder.addInput();
        auto inputIdxForTT = subnetBuilderForTT.addInput();

        linkArray[permutationVec.at(i)] = inputIdx.idx;
        linkArrayForTT[permutationVec.at(i)] = inputIdxForTT.idx;
      }

      std::vector<Link> linkList;
      std::vector<Link> linkListForTT;

      for (size_t i = 0; i < cellType.getInNum(); ++i) {
        linkList.emplace_back(linkArray[i]);
        linkListForTT.emplace_back(linkArrayForTT[i]);
      }

      auto cellIdx = subnetBuilder.addCell(cellTypeID, linkList);
      auto cellIdxForTT = subnetBuilderForTT.addSubnet(cellType.getImpl(),linkListForTT);

      subnetBuilder.addOutput(cellIdx);
      subnetBuilderForTT.addOutput(cellIdxForTT.at(0));

      SubnetID subnetID = subnetBuilder.make();

      subnets.push_back(subnetID);

      std::vector<Power> perPower;

      for (size_t i = 0; i < cellType.getInNum(); ++i) {
        perPower.push_back(powers.at(permutationVec.at(i)));
      }

      Subnetattr subnetattr{cellType.getName(), cellType.getAttr().props.area, perPower};
      subnetToAttr[subnetID] = subnetattr;

      ttSubnet[model::evaluate(model::Subnet::get(subnetBuilderForTT.make())).at(0)].push_back(subnetID);

    } while (std::next_permutation(permutationVec.begin(), permutationVec.end()));
  }
  assert(isFunctionallyComplete());
}

std::vector<SubnetID> CellDB::getSubnetIDsByTT(const kitty::dynamic_truth_table& tt) const {
  auto it = ttSubnet.find(tt);
  return (it != ttSubnet.end()) ? it->second : std::vector<SubnetID>{};
}

const Subnetattr &CellDB::getSubnetAttrBySubnetID(const SubnetID id) const {
  assert(subnetToAttr.find(id) != subnetToAttr.end());
  return subnetToAttr.at(id);
}

std::vector<SubnetID> &CellDB::getPatterns() {
  assert(subnetToAttr.size() != 0);
  if (patterns.size() != 0) {
    for (const auto &pair: subnetToAttr) {
      patterns.push_back(pair.first);
    }
  }
  return patterns;
}

kitty::dynamic_truth_table create_not(unsigned num_vars) {
  kitty::dynamic_truth_table tt(num_vars);
  kitty::create_nth_var(tt, 0);
  return ~tt;
}

kitty::dynamic_truth_table create_and(unsigned num_vars) {
  kitty::dynamic_truth_table tt(num_vars);
  kitty::create_from_binary_string(tt, "0001");
  return tt;
}

kitty::dynamic_truth_table create_or(unsigned num_vars) {
  kitty::dynamic_truth_table tt(num_vars);
  kitty::create_from_binary_string(tt, "0111");
  return tt;
}

kitty::dynamic_truth_table create_nand(unsigned num_vars) {
  return ~create_and(num_vars);
}

kitty::dynamic_truth_table create_nor(unsigned num_vars) {
  return ~create_or(num_vars);
}

kitty::dynamic_truth_table create_xor(unsigned num_vars) {
  kitty::dynamic_truth_table tt(num_vars);
  kitty::create_from_binary_string(tt, "0110");
  return tt;
}

kitty::dynamic_truth_table create_xnor(unsigned num_vars) {
  return ~create_xor(num_vars);
}

bool CellDB::isFunctionallyComplete() {

  unsigned num_vars = 2;

  auto tt_not = create_not(1);
  auto tt_and = create_and(num_vars);
  auto tt_or = create_or(num_vars);
  auto tt_nand = create_nand(num_vars);
  auto tt_nor = create_nor(num_vars);
  auto tt_xor = create_xor(num_vars);
  auto tt_xnor = create_xnor(num_vars);

  bool hasNot = ttSubnet.count(tt_not) > 0;
  bool hasAnd = ttSubnet.count(tt_and) > 0;
  bool hasOr = ttSubnet.count(tt_or) > 0;
  bool hasNand = ttSubnet.count(tt_nand) > 0;
  bool hasNor = ttSubnet.count(tt_nor) > 0;
  bool hasXor = ttSubnet.count(tt_xor) > 0;
  bool hasXnor = ttSubnet.count(tt_xnor) > 0;

  bool hasNonTruePreserving = hasNot || hasNand || hasNor || hasXor || hasXnor;
  bool hasNonFalsePreserving = hasAnd || hasOr || hasNand || hasNor || hasXor || hasXnor;
  bool hasNonMonotonic = hasXor || hasNand || hasNor || hasXnor;
  bool hasNonSelfDual = hasAnd || hasOr || hasNand || hasNor || hasNot || hasXor || hasXnor;

  if (hasNonTruePreserving && hasNonFalsePreserving && hasNonMonotonic && hasNonSelfDual) {
    return true;
  }

  if ((hasAnd && hasOr && hasNot) || hasNand || hasNor) {
    return true;
  }
  return false;
}

} // namespace eda::gate::techmapper
