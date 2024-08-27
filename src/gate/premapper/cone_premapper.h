//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/iomapping.h"
#include "gate/model/subnetview.h"
#include "gate/optimizer/mffc.h"
#include "gate/optimizer/reconvergence.h"
#include "gate/optimizer/resynthesizer.h"
#include "gate/optimizer/safe_passer.h"
#include "gate/optimizer/transformer.h"

namespace eda::gate::premapper {

enum Basis {AIG, XAG, MIG, XMG};

class ConePremapper : public optimizer::SubnetTransformer {
public:

  using Cell = gate::model::Subnet::Cell;
  using InOutMapping = gate::model::InOutMapping;
  using Link = gate::model::Subnet::Link;
  using ResynthesizerBase = optimizer::ResynthesizerBase;
  using SafePasser = gate::optimizer::SafePasser;
  using SubnetBuilder = gate::model::SubnetBuilder;
  using SubnetBuilderPtr = std::shared_ptr<SubnetBuilder>;
  using SubnetObject = gate::model::SubnetObject;
  using SubnetView = gate::model::SubnetView;

  ConePremapper(const std::string &name, Basis basis,
                const ResynthesizerBase &resynthesizer, uint16_t k):
      optimizer::SubnetTransformer(name), basis(basis),
      resynthesizer(resynthesizer), k(k) {

        arity = (basis == MIG) || (basis == XMG) ? 3 : 2;
        assert(k >= 3 && "The cut size is too small!");
      }

  SubnetBuilderPtr map(const SubnetBuilderPtr &builder) const override;

private:
  int resynthesize(const SubnetBuilder *builderPtr,
                   SafePasser &iter,
                   const SubnetView &view,
                   bool mayOptimize) const;

  void decomposeCell(const SubnetBuilderPtr &builder,
                     SafePasser &iter,
                     const model::EntryID entryID) const;

  void constantCase(const SubnetBuilderPtr &builder,
                    SafePasser &iter,
                    const model::EntryID entryID) const;

  const Basis basis;
  const ResynthesizerBase &resynthesizer;
  const uint16_t k;
  uint16_t arity;
};

} // namespace eda::gate::premapper
