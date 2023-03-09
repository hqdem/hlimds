//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cuts_finder_visitor.h"
#include "gate/optimizer/walker.h"

#include <queue>

class Optimizer {
  using GNet = eda::gate::model::GNet;
  using Vertex =  eda::gate::model::GNet::V;
  using Gate =  eda::gate::model::Gate;
  using Cut = std::unordered_set<Vertex>;

  GNet *gNet;
  CutStorage cutStorage;
public:
  Optimizer(GNet *gNet);
  void optimize(int cutSize);
  // bool isCut(const Vertex gate, const Cut &cut, Vertex &failed);

  const CutStorage& getCutStorage() {
    return cutStorage;
  }
};
