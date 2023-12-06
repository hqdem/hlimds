//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/cell.h"
#include "gate/model2/net.h"

#include <list>

  using LinkList = eda::gate::model::Cell::LinkList;
  using CellID = eda::gate::model::CellID;
  using NetID = eda::gate::model::NetID;

namespace eda::gate::tech_optimizer {

  //void mapDFF( LinkList linkList);
  //void mapLatch( LinkList linkList);
  //void mapDFFrs( LinkList linkList);
  std::list<CellID> getSequenceInputs(NetID netID, CellID subnetOutput);
} // namespace eda::gate::tech_optimizer
