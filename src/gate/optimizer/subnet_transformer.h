//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/optimizer/transformer.h"

namespace eda::gate::optimizer {

using SubnetBuilderPtr = BuilderPtr<model::SubnetBuilder>;

using SubnetTransformer = Transformer<model::SubnetBuilder>;

using SubnetInPlaceTransformer = InPlaceTransformer<model::SubnetBuilder>;

using SubnetInPlaceTransformerChain =
    InPlaceTransformerChain<model::SubnetBuilder>;

using SubnetPass = std::shared_ptr<SubnetInPlaceTransformer>;

using SubnetMapper = std::shared_ptr<SubnetTransformer>;

} // namespace eda::gate::optimizer
