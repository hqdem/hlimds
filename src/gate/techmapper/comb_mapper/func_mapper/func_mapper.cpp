//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techmapper/comb_mapper/func_mapper/func_mapper.h"

namespace eda::gate::techmapper {

using Type = MappingItem::Type;

void FuncMapper::addNotAnAndToTheMap(
       const EntryIndex index, const Cell &cell, Mapping &mapping) {
  if (cell.isIn()) {
    addInputToTheMap(index, mapping);
  } else if (cell.isOne()) {
    addOneToTheMap(index, mapping);
  } else if (cell.isZero()) {
    addZeroToTheMap(index, mapping);
  } else if (cell.isOut()) {
    addOutToTheMap(index, cell, mapping);
  }
}

void FuncMapper::addInputToTheMap(const EntryIndex index, Mapping &mapping) {
  MappingItem mappingItem(Type::IN);
  mapping[index] = mappingItem;
}

void FuncMapper::addZeroToTheMap(const EntryIndex index, Mapping &mapping) {
  MappingItem mappingItem(Type::ZERO);
  mapping[index] = mappingItem;
}

void FuncMapper::addOneToTheMap(const EntryIndex index, Mapping &mapping) {
  MappingItem mappingItem(Type::ONE);
  mapping[index] = mappingItem;
}

void FuncMapper::addOutToTheMap(
       const EntryIndex index, const Cell &cell, Mapping &mapping) {
  MappingItem mappingItem(Type::OUT);
  mappingItem.inputs.push_back(cell.link[0].idx);
  mapping[index] = mappingItem;
}
} // namespace eda::gate::techmapper
