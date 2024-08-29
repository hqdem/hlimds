//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnetview.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/techmapper/matcher/pbool_matcher.h"

namespace eda::gate::techmapper {

std::vector<SubnetTechMapper::Match> PBoolMatcher::match(
    const model::SubnetBuilder &builder,
    const optimizer::Cut &cut) {
  using SubnetView = model::SubnetView;

  std::vector<SubnetTechMapper::Match> matches;

  std::vector<size_t> entryIdxs(cut.leafIDs.begin(), cut.leafIDs.end());

  SubnetView cone(builder, cut);
  auto truthTable = cone.evaluateTruthTable();

  auto config = kitty::exact_p_canonization(truthTable);
  const auto &ctt = util::getTT(config); // canonized TT
  util::NpnTransformation t = util::getTransformation(config);

  if (auto it = cells.find(ctt); it != cells.end()) {
    for (const auto &cell : it->second) {
      model::Subnet::LinkList linkList(cell.transform.permutation.size());
      uint i = 0;
      for (const auto &index : t.permutation) {
        linkList[cell.transform.permutation.at(i++)] =
          model::Subnet::Link{(uint32_t)entryIdxs.at(index)};
      }
      matches.push_back(SubnetTechMapper::Match{cell.cellTypeID, linkList});
    }
  }

  return matches;
}

} // namespace eda::gate::techmapper
