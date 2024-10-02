//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/techmapper/matcher/matcher.h"
#include "util/kitty_utils.h" //For hash<kitty::dynamic_truth_table>

#include <kitty/kitty.hpp>

namespace eda::gate::techmapper {

class PBoolMatcher final : public Matcher<PBoolMatcher, kitty::dynamic_truth_table> {
  using StandardCell = library::StandardCell;
public:

  inline void match(
      std::vector<std::pair<StandardCell, uint16_t>> &scs,
      const kitty::dynamic_truth_table &ctt) {

    if (auto it = cells.find(ctt); it != cells.end()) {
      for (const auto &cell : it->second) {
        scs.push_back(cell);
      }
    }
  }

  std::vector<SubnetTechMapperBase::Match> match(
      const kitty::dynamic_truth_table &truthTable,
      const std::vector<model::EntryID> &entryIdxs);

  std::vector<SubnetTechMapperBase::Match> match(
      const model::SubnetBuilder &builder,
      const optimizer::Cut &cut,
      const bool constant) override;
};

} // namespace eda::gate::techmapper
