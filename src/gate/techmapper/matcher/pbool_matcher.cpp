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
using StandardCell = library::SCLibrary::StandardCell;

std::vector<SubnetTechMapperBase::Match> PBoolMatcher::match(
    const kitty::dynamic_truth_table &truthTable,
    const optimizer::Cut &cut) {
  std::vector<SubnetTechMapperBase::Match> matches;

  auto config = kitty::exact_p_canonization(truthTable);
  const auto &ctt = util::getTT(config); // canonized TT
  util::NpnTransformation t = util::getTransformation(config);
  std::vector<model::EntryID> entryIdxs(cut.leafIDs.begin(), cut.leafIDs.end());
  std::vector<StandardCell> scs;

  match(scs, ctt);

  for (const auto &cell : scs) {
    model::Subnet::LinkList linkList(cell.transform.permutation.size());
    uint i = 0;
    for (const auto &index : t.permutation) {
      linkList[cell.transform.permutation.at(i++)] =
        model::Subnet::Link{(uint32_t)entryIdxs.at(index)};
    }
    const auto match = SubnetTechMapperBase::Match{cell.cellTypeID, linkList};
    matches.push_back(match);
  }

  return matches;
}

std::vector<SubnetTechMapperBase::Match> PBoolMatcher::match(
    const model::SubnetBuilder &builder,
    const optimizer::Cut &cut) {
  model::SubnetView cone(builder, cut);
  auto truthTable = cone.evaluateTruthTable();

  return match(truthTable, cut);
}

} // namespace eda::gate::techmapper
