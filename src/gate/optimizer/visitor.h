//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cut_storage.h"

class Visitor {

public:

  using Vertex = eda::gate::model::GNet::V;
  using Cut = CutStorage::Cut;

  virtual void onGate(const Vertex&) = 0;
  virtual void onCut(const Cut&) = 0;
};
