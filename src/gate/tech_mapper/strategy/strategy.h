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
 #include "gate/tech_mapper/replacement_struct.h"

/**
 * \brief Interface to handle node and its cuts.
 * \author <a href="mailto:dreamer_1977@ispras.ru">Liza Shcherbakova</a>
 */
namespace eda::gate::techMap {
  using GateID = eda::gate::model::Gate::Id;
  class Strategy {
  public:
    virtual bool checkOpt(const eda::gate::optimizer::BoundGNet &,
        const eda::gate::model::GNet::GateIdMap &, double &,
        std::unordered_map<GateID, Replacement> *);
  };
} // namespace eda::gate::techMap