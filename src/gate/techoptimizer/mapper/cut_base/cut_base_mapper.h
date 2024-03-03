//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//
#pragma once

#include "gate/optimizer2/cut_extractor.h"
#include "gate/techoptimizer/mapper/baseMapper.h"

#include <map>

/**
 * \brief Interface to handle node and its cuts.
 * \author <a href="mailto:dGaryaev@ispras.ru">Daniil Gariaev</a>
 */

namespace eda::gate::tech_optimizer {
class CutBaseMapper : public BaseMapper {

protected:
  optimizer2::CutExtractor *cutExtractor;

  void baseMap() override;
  virtual void findBest() = 0;

  void addNotAnAndToTheMap(EntryIndex entryIndex,const model::Subnet::Cell &cell);

  void addInputToTheMap(EntryIndex entryIndex);
  void addZeroToTheMap(EntryIndex entryIndex);
  void addOneToTheMap(EntryIndex entryIndex);
  void addOutToTheMap(EntryIndex entryIndex,
                      const model::Subnet::Cell &cell);
};
} // namespace eda::gate::tech_optimizer