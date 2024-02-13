//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "gate/model/gsymbol.h"

#include <fstream>

using GateIdList = eda::gate::model::GNet::GateIdList;

class Dot {
public:
  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;
  using GateId = eda::gate::model::GNet::GateId;
  using GateSymbol = eda::gate::model::GateSymbol;

  Dot(const GNet *gNet);
  void fillColorGate(const std::string &filename, GateIdList &gateList);
  void print(const std::string &filename) const;
  void print(std::ofstream &stream) const;

private:
  GateIdList createList(GateIdList &gateList);
  const GNet *gNet;
  void print(std::ofstream &stream, const Gate *gate) const;
  void printGraph(std::ofstream &stream) const;
  static std::vector<std::string> funcNames;
};

