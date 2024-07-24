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
  using CellType = model::CellType;
  using CellTypeID = model::CellTypeID;
  using SubnetID = model::SubnetID;
  using StandardCell = library::SCLibrary::StandardCell;

public:
  static BaseType *create(const std::vector<StandardCell> &cells) {
    auto *instance = new BaseType();
    instance->initHash(cells);
    return instance;
  }

  virtual std::vector<SubnetTechMapper::Match> match(
    const model::SubnetBuilder &builder,
    const optimizer::CutExtractor::Cut &cut) = 0;

  virtual ~Matcher() = default;

  void initHash(const std::vector<StandardCell> &cells) {
    this->cells.clear();
    for (const auto& cell : cells) {
      model::SubnetBuilder subnetBuilder;

      std::vector<model::Subnet::Link> inputs;
      for (size_t i = 0; i < cell.link.size(); i++) {
        inputs.push_back(subnetBuilder.addInput());
      }
      std::vector<model::Subnet::Link> link;
      for (size_t i = 0; i < cell.link.size(); i++) {
        link.push_back(inputs.at(cell.link.at(i)));
      }
      const auto sCell = subnetBuilder.addSingleOutputSubnet(CellType::get(cell.cellTypeID).getImpl(), link);
      subnetBuilder.addOutput(sCell);

      KeyType key = makeHash(subnetBuilder.make());
      this->cells[key].push_back(cell);
    }
  }

protected:
  std::unordered_map<KeyType, std::vector<StandardCell>> cells;
  virtual KeyType makeHash(SubnetID subnetID) = 0;
};

} // namespace eda::gate::techmapper