//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library.h"
#include "gate/library/library_characteristics.h"
#include "gate/optimizer/synthesis/isop.h"

namespace eda::gate::library {
using CellTypeID = model::CellTypeID;
using MinatoMorrealeAlg = optimizer::synthesis::MMSynthesizer;

void SCLibrary::loadCells() {
  combCells.clear();

  for (const auto &cell : LibraryCharacteristics::getCells()) {
    if (LibraryCharacteristics::isIsolateCell(cell)) continue;

    if (LibraryCharacteristics::isCombCell(cell)) {
      addCombCell(cell);
    } else {
      //addSeqCell(cell);
    }
  }
  permutation();
}

void SCLibrary::addCell(CellTypeID cell) {
  //combCells.push_back(cell);
}

std::vector<SCLibrary::StandardCell> SCLibrary::getCombCells() {
  return combCells;
}

void SCLibrary::addCombCell(std::string cell) {
  const auto typeAttr = model::makeCellTypeAttr();
  model::CellTypeAttr::get(typeAttr).props.area =
      LibraryCharacteristics::getArea(cell);

  if (LibraryCharacteristics::getInputs(cell).empty() ||
      LibraryCharacteristics::getArea(cell) == MAXFLOAT) {
    return;
  }

  model::CellProperties props(true, false, true, false, false, false,
                              false, false, false);

  MinatoMorrealeAlg minatoMorrealeAlg;
  auto subnetObject = minatoMorrealeAlg.synthesize(LibraryCharacteristics::getFunction(cell));

  const auto cellTypeID = model::makeCellType(
      model::CellSymbol::UNDEF, cell, subnetObject.make(),
      typeAttr, props, LibraryCharacteristics::getInputs(cell).size(),
      static_cast<uint16_t>(1)
  );

  std::vector<int> link = {};
  for (size_t i = 0; i < LibraryCharacteristics::getInputs(cell).size(); i++) {
    link.push_back(i);
  }
  combCells.push_back({cellTypeID, link});
}

void SCLibrary::permutation() {
  std::vector<StandardCell> perCells;
  for (size_t i = 0; i < combCells.size(); i++) {
    auto link = combCells.at(i).link;
    std::sort(link.begin(), link.end());
    while (std::next_permutation(link.begin(), link.end())) {
      perCells.push_back({combCells.at(i).cellTypeID, link});
    }
  }
  for (const auto &cell : perCells) {
    combCells.push_back(cell);
  }
}
} // namespace eda::gate::library