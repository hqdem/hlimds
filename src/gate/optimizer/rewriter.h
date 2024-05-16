//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/object.h"
#include "gate/optimizer/cone_builder.h"
#include "gate/optimizer/resynthesizer.h"
#include "gate/optimizer/safe_passer.h"
#include "gate/optimizer/transformer.h"

#include <functional>
#include <string>

namespace eda::gate::optimizer {

/**
 * @brief Finds and applies the best rewritings on each node according to the
 * number of elements in the old and resynthesized cones.
 */
class Rewriter final : public SubnetInPlaceTransformer {
public:
  using Subnet = eda::gate::model::Subnet;
  using SubnetID = eda::gate::model::SubnetID;
  using LinkList = Subnet::LinkList;
  using SubnetBuilder = eda::gate::model::SubnetBuilder;
  using Effect = SubnetBuilder::Effect;
  using SafePasser = eda::gate::optimizer::SafePasser;
  using CutExtractor = eda::gate::optimizer::CutExtractor;
  using ConeBuilder = eda::gate::optimizer::ConeBuilder;
  using ResynthesizerBase = eda::gate::optimizer::ResynthesizerBase;

  /**
   * @brief Constructs a rewriter.
   *
   * @param name Name of the rewriter.
   * @param resynthesizer Resythesizer used to synthesize new cone for each cut.
   * @param k Maximum number of elements in the cut.
   * @param cost Function that calculates the overall metric after replacement.
   * A greater returned value is a better result of the replacement.
   * @param zero_cost Enables zero-cost replacements if set.
   */
  Rewriter(
      const std::string &name,
      const ResynthesizerBase &resynthesizer,
      const unsigned k,
      const std::function<float(const Effect &)> cost,
      const bool zero_cost = false) :
    SubnetInPlaceTransformer(name),
    resynthesizer(resynthesizer), k(k), cost(cost), zero_cost(zero_cost) {}

  /**
   * @brief Rewrites the subnet stored in the builder by applying the
   * resynthesizer to k-feasible cuts and choosing the best subnet.
   *
   * @param builder SubnetBuilder with the subnet to rewrite.
   */
  void transform(SubnetBuilder &builder) const override;

private:
  void rewriteOnNode(
      SubnetBuilder &builder,
      SafePasser &iter,
      CutExtractor &cutExtractor) const;

  const ResynthesizerBase &resynthesizer;
  const unsigned k;
  const std::function<float(const Effect &)> cost;
  const bool zero_cost;

  constexpr static float metricEps = 1e-6;
};

} // namespace eda::gate::optimizer
