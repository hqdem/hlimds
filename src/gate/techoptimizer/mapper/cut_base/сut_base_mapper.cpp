//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techoptimizer/mapper/cut_base/cut_base_mapper.h"

namespace eda::gate::tech_optimizer {
void CutBaseMapper::baseMap() {
  std::cout << "Find Cuts" << std::endl;
  auto startCut = std::chrono::high_resolution_clock::now();
  cutExtractor = new optimizer2::CutExtractor(&model::Subnet::get(subnetID), 6);
  auto endCut = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> CutFindTime = endCut - startCut;
  std::cout << "Функция CutExtractor выполнялась " << CutFindTime.count() << " секунд.\n";
  findBest();
}
void CutBaseMapper::addNotAnAndToTheMap(EntryIndex entryIndex, model::Subnet::Cell &cell) {
  if (cell.isIn()) {
    addInputToTheMap(entryIndex);
  } else if (cell.isOne()) {
    addOneToTheMap(entryIndex);
  } else if (cell.isZero()) {
    addZeroToTheMap(entryIndex);
  } else if (cell.isOut()) {
    addOutToTheMap(entryIndex, cell);
  }
}
void CutBaseMapper::addInputToTheMap(EntryIndex entryIndex) {
  BestReplacement bestReplacement{true};
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}
void CutBaseMapper::addZeroToTheMap(EntryIndex entryIndex) {
  BestReplacement bestReplacement{};
  bestReplacement.isZero = true;
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}
void CutBaseMapper::addOneToTheMap(EntryIndex entryIndex) {
  BestReplacement bestReplacement{};
  bestReplacement.isOne = true;
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}
void CutBaseMapper::addOutToTheMap(EntryIndex entryIndex,
                                      model::Subnet::Cell &cell) {
  BestReplacement bestReplacement{false, true};
  bestReplacement.entryIDxs.insert(cell.link[0].idx);
  (*bestReplacementMap)[entryIndex] = bestReplacement;
}
} // namespace eda::gate::tech_optimizer