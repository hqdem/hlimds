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

SCLibrary::SCLibrary(const std::string &fileName) {
  // TODO: if we want to avoid usage FILE, we need to fix it in ReadCells.
  FILE *file = fopen(fileName.c_str(), "rb");
  if (file != nullptr) {
    ast = tokParser.parseLibrary(file, fileName.c_str());

    AstParser parser(library, tokParser);
    parser.run(*ast);
    fclose(file);

    iface = new ReadCellsIface(library);

    for (const auto &name : iface->getCells()) {
      if (iface->isIsolateCell(name)) continue;

      if (iface->isCombCell(name)) {
        loadCombCell(name);
      } else {
        //loadSeqCell(cell);
      }
    }
  }
}

SCLibrary::~SCLibrary() {
  if (iface != nullptr) {
    delete iface;
  }
}

void SCLibrary::loadCombCell(const std::string &name) {
  const auto ports = iface->getPorts(name);

  const auto nInputs = model::CellTypeAttr::getInBitWidth(ports);
  const auto nOutputs = model::CellTypeAttr::getOutBitWidth(ports);

  const auto props = iface->getPhysProps(name);

  if (nInputs == 0 || props.area == MAXFLOAT) {
    return;
  }

  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(props);

  const auto func = iface->getFunction(name);
  auto subnetObject = optimizer::synthesis::MMSynthesizer{}.synthesize(func);
  const auto subnetID = subnetObject.make();

  const auto cellTypeID = model::makeCellType(
      model::CellSymbol::UNDEF,
      name,
      subnetID,
      attrID,
      model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
      nInputs,
      nOutputs);

  std::vector<int> links(nInputs);
  std::iota(std::begin(links), std::end(links), 0);

  auto config = kitty::exact_p_canonization(func);
  const auto &ctt = utils::getTT(config); // canonized TT
  utils::NpnTransformation t = utils::getTransformation(config);

  combCells.push_back({cellTypeID, links, ctt, t});
}

SCLibrary *library = nullptr;

} // namespace eda::gate::library
