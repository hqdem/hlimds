//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/function/truth_table.h"
#include "gate/techmapper/matcher/matcher.h"

namespace eda::gate::techmapper {

class PBoolMatcher final : public Matcher<PBoolMatcher, model::TruthTable> {
  using StandardCell = library::StandardCell;

public:
  inline void match(
      std::vector<std::pair<StandardCell, uint16_t>> &scs,
      const model::TruthTable &ctt) {

    if (auto it = cells.find(ctt); it != cells.end()) {
      for (const auto &cell : it->second) {
        scs.push_back(cell);
      }
    }
  }

  std::vector<SubnetTechMapperBase::Match> match(
      const model::TruthTable &truthTable,
      const std::vector<model::EntryID> &entryIdxs);

  std::vector<SubnetTechMapperBase::Match> match(
      const std::shared_ptr<model::SubnetBuilder> &builder,
      const optimizer::Cut &cut) override;
};

} // namespace eda::gate::techmapper
