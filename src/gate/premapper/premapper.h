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
#include "gate/optimizer/subnet_transformer.h"

namespace eda::gate::premapper {

enum Basis {AIG, XAG, MIG, XMG};

class Premapper : public optimizer::SubnetTransformer {
public:
 
  using Cell = gate::model::Subnet::Cell;
  using InOutMapping = gate::model::InOutMapping;
  using Link = gate::model::Subnet::Link;
  using SafePasser = gate::optimizer::SafePasser;
  using SubnetBuilder = gate::model::SubnetBuilder;
  using SubnetBuilderPtr = gate::optimizer::SubnetBuilderPtr;
  using SubnetObject = gate::model::SubnetObject;

  Premapper(const std::string &name, Basis basis,
            const optimizer::ResynthesizerBase &resynthesizer, uint16_t k = 4):
      optimizer::SubnetTransformer(name), basis(basis),
      resynthesizer(resynthesizer), k(k) {
        arity = (basis == MIG) || (basis == XMG) ? 3 : 2;
        assert(k >= 3 && "The cut size is too small!");
      }

  SubnetBuilderPtr map(const SubnetBuilderPtr &builder) const override;

private:
  void decomposeCell(const SubnetBuilderPtr &builder,
                     SafePasser &iter,
                     const size_t entryID) const;

  void constantCase(const SubnetBuilderPtr &builder,
                    SafePasser &iter,
                    const size_t entryID) const;

  const Basis basis;
  const optimizer::ResynthesizerBase &resynthesizer;
  const uint16_t k;
  uint16_t arity;
};

} // namespace eda::gate::premapper
