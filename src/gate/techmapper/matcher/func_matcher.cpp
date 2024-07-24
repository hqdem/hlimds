//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/matcher/func_matcher.h"
#include "gate/model/subnetview.h"
#include "gate/model/utils/subnet_truth_table.h"

namespace eda::gate::techmapper {

std::size_t FuncMatcher::hash_dynamic_tt(
    const kitty::dynamic_truth_table &dtt) {
  std::size_t hash = 0;
  for (auto block : dtt) {
    hash ^= std::hash<uint64_t>{}(block) + 0x9e3779b9 +
            (hash << 6) + (hash >> 2);
  }
  return hash;
}

std::vector<SubnetTechMapper::Match> FuncMatcher::match(
    const model::SubnetBuilder &builder,
    const optimizer::CutExtractor::Cut &cut) {
  using SubnetView = model::SubnetView;

  std::vector<SubnetTechMapper::Match> matches;
  std::vector<size_t> entryIdxs(cut.entryIdxs.begin(), cut.entryIdxs.end());
  SubnetView cone(builder, cut);
  auto truthTable =  cone.evaluateTruthTable();

  auto key = hash_dynamic_tt(truthTable);

  if (cells.find(key) != cells.end()) {
    for (const auto &cell : cells[key]) {
      if (cell.link.size() != cut.entryIdxs.size()) continue;

      Subnet::LinkList linkList;
      for (const auto &inputID : cell.link) {
        linkList.push_back(Subnet::Link{(uint32_t)entryIdxs.at(inputID)});
      }
      matches.push_back(SubnetTechMapper::Match{cell.cellTypeID, linkList});
    }
  }
  return matches;
}

std::size_t FuncMatcher::makeHash(model::SubnetID subnetID) {
  auto &subnet = Subnet::get(subnetID);
  auto truthTable = model::evaluateSingleOut(subnet);
  return hash_dynamic_tt(truthTable);
}

} // namespace eda::gate::techmapper