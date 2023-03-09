//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/cut_storage.h"
#include "gate/optimizer/visitor.h"

class CutsFindVisitor : public Visitor {

  int cutSize;
  CutStorage *cutStorage;

public:

  CutsFindVisitor(int cutSize, CutStorage *cutStorage);
  void onGate(const Vertex&) override;
  void onCut(const Cut&) override;
};
