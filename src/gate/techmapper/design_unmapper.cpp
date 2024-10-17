//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
 
#include "gate/techmapper/design_unmapper.h"
#include "gate/techmapper/subnet_unmapper.h"

namespace eda::gate::techmapper {

void DesignUnmapper::transform(const DesignBuilderPtr &builder) const {
  optimizer::EachSubnetTransformer unmapper(
      std::make_shared<SubnetUnmapper>("unmap"));
  unmapper.transform(builder);

  // TODO: Unmap flip-flops and latches.
}

} // namespace eda::gate::techmapper
