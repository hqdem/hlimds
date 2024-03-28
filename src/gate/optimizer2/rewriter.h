//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer2/cone_builder.h"
#include "gate/optimizer2/transformer.h"
#include "gate/optimizer2/resynthesizer.h"

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
  using CutExtractor = eda::gate::optimizer2::CutExtractor;
  using ConeBuilder = eda::gate::optimizer2::ConeBuilder;
  using ResynthesizerBase = eda::gate::optimizer2::ResynthesizerBase;

  /**
   * @brief Constructs a rewriter.
   *
   * @param resynthesizer Resythesizer used to synthesize new cone for each cut.
   * @param k Maximum number of elements in the cut.
   */
  Rewriter(const ResynthesizerBase &resynthesizer, const unsigned k):
    resynthesizer(resynthesizer), k(k) {}

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
      const size_t entryID,
      CutExtractor &cutExtractor) const;

  const ResynthesizerBase &resynthesizer;
  const unsigned k;
};

} // namespace eda::gate::optimizer2
