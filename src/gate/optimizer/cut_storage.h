//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"

#include <unordered_map>

struct CutStorage {
  using GNet = eda::gate::model::GNet;
  using Vertex = GNet::V;

  struct HashFunction {
    size_t operator()(const std::unordered_set<Vertex>& set) const {
      std::hash<int> hasher;
      size_t answer = 0;

      for (int i: set) {
        answer ^= hasher(i) + 0x9e3779b9 +
                  (answer << 6) + (answer >> 2);
      }
      return answer;
    }
  };

  using Cut = std::unordered_set<Vertex>;
  using Cuts = std::unordered_set<Cut, HashFunction>;

  std::unordered_map<GNet::V, Cuts> cuts;
};
