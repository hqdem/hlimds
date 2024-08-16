//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/library.h"
#include "gate/model/celltype.h"
#include "gate/model/subnet.h"
#include "gate/optimizer/cut_extractor.h"
#include "gate/techmapper/subnet_techmapper.h"

#include <functional>
#include <vector>

namespace eda::gate::techmapper {

template<typename BaseType, typename KeyType>
class Matcher {
  using StandardCell = library::SCLibrary::StandardCell;

public:
  static BaseType *create(const std::vector<StandardCell> &cells) {
    auto *instance = new BaseType();
    instance->initMap(cells);
    return instance;
  }

  virtual std::vector<SubnetTechMapper::Match> match(
    const model::SubnetBuilder &builder,
    const optimizer::CutExtractor::Cut &cut) = 0;

  virtual ~Matcher() = default;

  void initMap(const std::vector<StandardCell> &cells) {
    for (const auto& cell : cells) {
      this->cells[cell.ctt].push_back(cell);
    }
  }

protected:
  std::unordered_map<KeyType, std::vector<StandardCell>> cells;
};

} // namespace eda::gate::techmapper