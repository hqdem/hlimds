//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/techmapper/comb_mapper/comb_mapper.h"

namespace eda::gate::techmapper {

class FuncMapper : public CombMapper {
  using Cell = model::Subnet::Cell;

public:
  virtual ~FuncMapper() = default;

protected:
  void addNotAnAndToTheMap(
         const EntryIndex index, const Cell &cell, Mapping &mapping);
  void addInputToTheMap(const EntryIndex index, Mapping &mapping);
  void addZeroToTheMap(const EntryIndex index, Mapping &mapping);
  void addOneToTheMap(const EntryIndex index, Mapping &mapping);
  void addOutToTheMap(
         const EntryIndex index, const Cell &cell, Mapping &mapping);
};
} // namespace eda::gate::techmapper
