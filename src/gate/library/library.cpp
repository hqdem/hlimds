//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library.h"
#include "gate/optimizer/synthesis/isop.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>

namespace eda::gate::library {

using CellTypeID = model::CellTypeID;
using MinatoMorrealeAlg = optimizer::synthesis::MMSynthesizer;

SCLibrary::SCLibrary(const std::string &fileName) {
  // TODO: if we want to avoid usage FILE, we need to fix it in ReadCells.
  FILE *file = fopen(fileName.c_str(), "rb");
  if (file != nullptr) {
    ast = tokParser.parseLibrary(file, fileName.c_str());

    AstParser parser(library, tokParser);
    parser.run(*ast);
    fclose(file);

    iface = new ReadCellsIface(library);
    loadCells();
  }
}

SCLibrary::~SCLibrary() {
  if (iface != nullptr) {
    delete iface;
  }
}

void SCLibrary::loadCells() {
  for (const auto &name : iface->getCells()) {
    if (iface->isIsolateCell(name)) continue;

    if (iface->isCombCell(name)) {
      addCombCell(name);
    } else {
      //addSeqCell(cell);
    }
  }
  permutation();
}

void SCLibrary::addCell(CellTypeID typeID) {
  //combCells.push_back(cell);
}

std::vector<SCLibrary::StandardCell> SCLibrary::getCombCells() {
  return combCells;
}

void SCLibrary::addCombCell(const std::string &name) {
  const auto ports = iface->getPorts(name);

  const auto nIn = model::CellTypeAttr::getInBitWidth(ports);
  const auto nOut = model::CellTypeAttr::getOutBitWidth(ports);

  const auto props = iface->getPhysProps(name);

  if (nIn == 0 || props.area == MAXFLOAT) {
    return;
  }

  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(props);

  const auto func = iface->getFunction(name);
  auto subnetObject = MinatoMorrealeAlg{}.synthesize(func);

  const auto cellTypeID = model::makeCellType(
      model::CellSymbol::UNDEF,
      name,
      subnetObject.make(),
      attrID,
      model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
      nIn,
      nOut);

  std::vector<int> link{};
  for (size_t i = 0; i < nIn; i++) {
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

SCLibrary *library = nullptr;

} // namespace eda::gate::library
