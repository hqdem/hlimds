//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/utils/subnet_truth_table.h"
#include "gate/techmapper/library/cell_db.h"
#include "gate/techmapper/library/check_lib/aig/check_lib_for_aig.h"
#include "gate/techmapper/library/liberty_manager.h"

namespace eda::gate::techmapper {

using CellSymbol = eda::gate::model::CellSymbol;
using CellType = eda::gate::model::CellType;
using Link = eda::gate::model::Subnet::Link;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using SubnetID = eda::gate::model::SubnetID;

CellDB::CellDB(const std::vector<CellTypeID> &cellTypeIDs,
               const std::vector<CellTypeID> &cellTypeFFIDs,
               const std::vector<CellTypeID> &cellTypeFFrsIDs,
               const std::vector<CellTypeID> &cellTypeLatchIDs) {

  std::cout << "The number of extracted from Liberty CellTypes is " <<
    cellTypeIDs.size() << std::endl; // TODO
  int count = 0;
  for (const CellTypeID &cellTypeID : cellTypeIDs) {
    CellType &cellType = CellType::get(cellTypeID);

    bool needPower = true;
    std::vector<Power> powers;
    if (needPower) {
      std::string cell_name = cellType.getName();

      const auto *cell = LibraryManager::get().getLibrary().getCell(cell_name);

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

      std::vector<Power> perPower;

      for (size_t i = 0; i < cellType.getInNum(); ++i) {
        perPower.push_back(powers.at(permutationVec.at(i)));
      }

      Subnetattr subnetattr{cellType.getName(), cellType.getAttr().props.area, perPower};
      subnetToAttr[subnetID] = subnetattr;
      ttSubnet[model::evaluate(cellType.getSubnet()).at(0)] = subnetID;
    } while (std::next_permutation(permutationVec.begin(), permutationVec.end()));
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

    Subnetattr subnetattr{"FF", cellType.getAttr().props.area};
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

    Subnetattr subnetattr{"FFrs", cellType.getAttr().props.area};
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

    Subnetattr subnetattr{"Latch", cellType.getAttr().props.area};
    Latch.emplace_back(subnetID, subnetattr);
  }
  std::cout << "The number of extracted from Liberty Subnets is " <<
    count << std::endl; // TODO

  AIGCheckerLiberty aigCheckerLiberty;
  std::vector<std::string> missing = aigCheckerLiberty.checkLiberty(ttSubnet);
  if (missing.size() != 0) {
    std::cout << "Missing expressions:" << std::endl;
    for (const auto &expr: missing) {
      std::cout << expr << std::endl;
    }
    assert(missing.size() == 0);
  }
}

std::vector<SubnetID> CellDB::getSubnetIDsByTT(const kitty::dynamic_truth_table& tt) const {
  std::vector<SubnetID> ids;
  auto range = ttSubnet.equal_range(tt);
  for (auto it = range.first; it != range.second; ++it) {
    ids.push_back(it->second);
  }
  return ids;
}

const Subnetattr &CellDB::getSubnetAttrBySubnetID(const SubnetID id) const {
  return subnetToAttr.at(id);
}

std::vector<SubnetID> CellDB::getPatterns() {
  std::vector<SubnetID> patterns;
  for (const auto &pair: subnetToAttr) {
    patterns.push_back(pair.first);
  }
  return patterns;
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

} // namespace eda::gate::techmapper
