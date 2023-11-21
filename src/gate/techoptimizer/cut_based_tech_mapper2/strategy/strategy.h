//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/model/gnet.h"
#include "gate/optimizer/rwdatabase.h"
 #include "gate/techoptimizer/cut_based_tech_mapper/replacement_struct.h"

/**
 * \brief Interface to handle node and its cuts.
 * \author <a href="mailto:dGaryaev@ispras.ru">Daniil Gariaev</a>
 */
namespace eda::gate::tech_optimizer {
  using GateID = eda::gate::model::Gate::Id;
  class Strategy2 {
  public:
    Strategy2() {};
    virtual bool checkOpt() = 0;
  };
} // namespace eda::gate::tech_optimizer