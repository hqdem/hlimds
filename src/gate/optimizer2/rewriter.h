//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"
#include "gate/optimizer2/cone_builder.h"
#include "gate/optimizer2/resynthesizer.h"
#include "gate/optimizer2/safe_passer.h"
#include "gate/optimizer2/transformer.h"

namespace eda::gate::optimizer2 {

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
  using SafePasser = eda::gate::optimizer2::SafePasser;
  using CutExtractor = eda::gate::optimizer2::CutExtractor;
  using ConeBuilder = eda::gate::optimizer2::ConeBuilder;
  using ResynthesizerBase = eda::gate::optimizer2::ResynthesizerBase;

  /**
   * @brief Constructs a rewriter.
   *
   * @param resynthesizer Resythesizer used to synthesize new cone for each cut.
   * @param k Maximum number of elements in the cut.
   * @param cost Function that calculates the overall metric after replacement.
   * A greater returned value is a better result of the replacement.
   */
  Rewriter(
      const ResynthesizerBase &resynthesizer,
      const unsigned k,
      const std::function<float(const Effect &)> &cost) :
    resynthesizer(resynthesizer), k(k), cost(cost) {}

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
  const std::function<float(const Effect &)> &cost;
  constexpr static float metricEps = 1e-6;
};

} // namespace eda::gate::optimizer2
