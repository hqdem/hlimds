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
#include "gate/techmapper/subnet_techmapper_base.h"

#include <functional>
#include <vector>

namespace eda::gate::techmapper {

template<typename BaseType, typename KeyType>
class Matcher {
  using StandardCell = library::StandardCell;

public:
  static std::unique_ptr<BaseType> create(const std::vector<StandardCell> &cells) {
    auto instance = std::make_unique<BaseType>();
    instance->initMap(cells);
    return instance;
  }

  virtual std::vector<SubnetTechMapperBase::Match> match(
    const model::SubnetBuilder &builder,
    const optimizer::Cut &cut,
    const bool constant) = 0;

  virtual ~Matcher() = default;

  void initMap(const std::vector<StandardCell> &cells) {
    for (const auto& cell : cells) {
      uint16_t output = 0;
      for (const auto& ctt : cell.ctt) {
        this->cells[ctt].push_back(std::pair{cell, output++});
      }
    }
  }

protected:
  std::unordered_map<KeyType,
    std::vector<std::pair<StandardCell, uint16_t>>> cells;
};

} // namespace eda::gate::techmapper
