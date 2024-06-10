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
  using CellTypeID = model::CellTypeID;
  using Subnet = model::Subnet;
  using SubnetID = model::SubnetID;
  using SubnetBuilder = model::SubnetBuilder;
  using SubnetBuilderPtr = optimizer::SubnetBuilderPtr;
  using Cut = optimizer::CutExtractor::Cut;
  using Cuts = optimizer::CutExtractor::CutsList;
  using CellTypeIDs = std::vector<CellTypeID>;
  using Criterion = optimizer::Criterion;
  using CostVector = optimizer::CostVector;
  using CostVectors = std::vector<CostVector>;

  using CutProvider = std::function<Cuts(const Subnet &, const size_t)>;
  using CellTypeProvider = std::function<CellTypeIDs(const Subnet &, const Cut &)>;
  using CellTypeEstimator = std::function<CostVector(const CellTypeID)>;
  using CostVectorAggregator = std::function<CostVector(const CostVectors &)>;

  SubnetTechMapper(const std::string &name,
                   const Criterion &criterion,
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

  SubnetBuilderPtr make(const SubnetID subnetID) const override;

private:
  const Criterion &criterion;
  const CutProvider cutProvider;
  const CellTypeProvider cellTypeProvider;
  const CellTypeEstimator cellTypeEstimator;
  const CostVectorAggregator flowCostAggregator;
  const CostVectorAggregator exactCostAggregator;
};

} // namespace eda::gate::techmapper
