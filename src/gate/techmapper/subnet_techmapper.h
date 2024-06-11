//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/criterion/criterion.h"
#include "gate/optimizer/criterion/solution_space.h"
#include "gate/optimizer/cut_extractor.h"
#include "gate/optimizer/subnet_transformer.h"

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace eda::gate::techmapper {

/**
 * @brief General dynamic programming-based subnet mapper.
 */
class SubnetTechMapper final : public optimizer::SubnetTransformer {
public:
  struct Match final {
    model::CellTypeID typeID{model::OBJ_NULL_ID};
    model::Subnet::LinkList links{};
    bool inversion{false};
  };

  using Cut = optimizer::CutExtractor::Cut;
  using Cuts = optimizer::CutExtractor::CutsList;

  using CutProvider =
      std::function<Cuts(const model::Subnet &, const size_t)>;
  using CellTypeProvider =
      std::function<std::vector<Match>(const model::Subnet &, const Cut &)>;
  using CellTypeEstimator =
      std::function<optimizer::CostVector(const model::CellTypeID)>;
  using CostVectorAggregator =
      std::function<optimizer::CostVector(const std::vector<optimizer::CostVector> &)>;

  SubnetTechMapper(const std::string &name,
                   const optimizer::Criterion &criterion,
                   const CutProvider cutProvider,
                   const CellTypeProvider cellTypeProvider,
                   const CellTypeEstimator cellTypeEstimator,
                   const CostVectorAggregator flowCostAggregator,
                   const CostVectorAggregator exactCostAggregator):
      optimizer::SubnetTransformer(name),
      criterion(criterion),
      cutProvider(cutProvider),
      cellTypeProvider(cellTypeProvider),
      cellTypeEstimator(cellTypeEstimator),
      flowCostAggregator(flowCostAggregator),
      exactCostAggregator(exactCostAggregator) {}

  optimizer::SubnetBuilderPtr make(
      const model::SubnetID subnetID) const override;

private:
  const optimizer::Criterion &criterion;
  const CutProvider cutProvider;
  const CellTypeProvider cellTypeProvider;
  const CellTypeEstimator cellTypeEstimator;
  const CostVectorAggregator flowCostAggregator;
  const CostVectorAggregator exactCostAggregator;
};

} // namespace eda::gate::techmapper
