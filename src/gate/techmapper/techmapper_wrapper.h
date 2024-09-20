//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "context/utopia_context.h"
#include "gate/criterion/criterion.h"
#include "gate/model/subnet.h"

namespace eda::gate::techmapper {

std::shared_ptr<model::SubnetBuilder> techMap(
  const criterion::Objective objective,
  const std::shared_ptr<model::SubnetBuilder> &builder
);

class techMapperWrapper {
public:
  struct techMapResult {
    bool success = true;
    size_t failedSubnet = -1;
  };
  
  //not movable, bot not copyable
  techMapperWrapper (techMapperWrapper &&) = delete;
  techMapperWrapper &  operator= (techMapperWrapper &&) = delete;
  techMapperWrapper (const techMapperWrapper &) = delete;
  techMapperWrapper & operator= (const techMapperWrapper &) = delete;

  techMapperWrapper(context::UtopiaContext &context,
                    model::DesignBuilder &design)
                    : context_(context), design_(design) {}

  techMapResult techMap();

private:
  std::shared_ptr<model::SubnetBuilder> generateTechSubnet(
    const SubnetTechMapperBase::SubnetBuilderPtr &builder);

  context::UtopiaContext &context_;
  model::DesignBuilder &design_;
};

} // namespace eda::gate::techmapper
