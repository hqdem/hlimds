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

namespace eda::gate::optimizer {

/**
 * \brief Class for storing cuts.
 */
  struct CutStorage {
    using GNet = model::GNet;
    using GateId = GNet::GateId;
    using Cut = std::unordered_set<GateId>;

    struct HashFunction {
      size_t operator()(const Cut &cut) const {
        std::hash<int> hasher;
        size_t answer = 0;

        for (int i: cut) {
          answer ^= hasher(i) + 0x9e3779b9 + (answer << 6) + (answer >> 2);
        }
        return answer;
      }
    };

    using Cuts = std::unordered_set<Cut, HashFunction>;

    std::unordered_map<GateId, Cuts> cuts;
  };
} // namespace eda::gate::optimizer
