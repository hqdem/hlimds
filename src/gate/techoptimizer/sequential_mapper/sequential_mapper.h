//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"

namespace eda::gate::tech_optimizer {
  using GNet = eda::gate::model::GNet;
  using LinkList = model::Cell::LinkList;

  void mapDFF( LinkList linkList);
  void mapLatch( LinkList linkList);
  void mapDFFrs( LinkList linkList);
  void tech_optimize(GNet *net, uint approachSelector/*, Constraints &constraints*/);
} // namespace eda::gate::tech_optimizer
