//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/techmapper/matcher/matcher.h"
#include <kitty/kitty.hpp>

namespace eda::gate::techmapper {

class FuncMatcher final : public Matcher<FuncMatcher, std::size_t> {
  using Subnet = model::Subnet;
public:
  ~FuncMatcher() = default;

  std::vector<SubnetTechMapper::Match> match(
      const model::SubnetBuilder &builder,
      const optimizer::CutExtractor::Cut &cut) override;

private:
  std::size_t makeHash(model::SubnetID subnetID) override;
  std::size_t hash_dynamic_tt(const kitty::dynamic_truth_table &dtt);
};

} // namespace eda::gate::techmapper
