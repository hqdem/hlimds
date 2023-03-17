//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/optimizer/cut_storage.h"
#include "gate/optimizer/visitor.h"
#include "util/graph.h"

class Walker {
  using GNet = eda::gate::model::GNet;

  GNet *gNet;
  Visitor *visitor;
  CutStorage *cutStorage;
public:
  Walker(GNet *gNet, Visitor *visitor, CutStorage* cutStorage);

  void walk(bool forward);
};

