//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/estimator/npn_estimator.h"
#include "gate/model/subnetview.h"
#include "gate/optimizer/cut_extractor.h"
#include "gate/optimizer/safe_passer.h"

namespace eda::gate::estimator {

using CutExtractor = gate::optimizer::CutExtractor;
using SafePasser   = gate::optimizer::SafePasser;
using SubnetView   = gate::model::SubnetView;

void NpnEstimator::estimate(const SubnetBuilderPtr &builder,
                            const NpnSettings &settings,
                            NpnStats &result) const {

  const auto k = settings.k;
  CutExtractor extractor(builder.get(), k, true);
  for (SafePasser iter = builder->begin();
       !builder->getCell(*iter).isOut() && (iter != builder->end());
       ++iter) {

    if (builder->getCell(*iter).isBuf()) {
      continue;
    }

    const auto &cuts = extractor.getCuts(*iter);
    for (const auto &cut : cuts) {
      const auto nVars = cut.leafIDs.size();
      if (!settings.extendTables && (nVars != k)) {
        continue;
      }
      if (!settings.countTrivial && cut.isTrivial()) {
        continue;
      }

      SubnetView cone(*(builder.get()), cut);
      auto tt = cone.evaluateTruthTable();
      auto ttk = nVars < k ? kitty::extend_to(tt, k) : tt;

      const auto config = kitty::exact_npn_canonization(ttk);
      ttk = util::getTT(config);

      if (result.find(ttk) == result.end()) {
        result[ttk] = 1;
      } else {
        ++result[ttk];
      }
    }
  }
}

} // namespace eda::gate::estimator
