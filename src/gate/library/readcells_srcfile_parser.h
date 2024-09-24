//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once
#include "gate/library/cell_srcfile_parser_iface.h"
#include <readcells/groups.h> //FIXME: try to move this to cpp file
#include <readcells/token_parser.h> //FIXME: try to move this to cpp file
#include <string>
#include <vector>

//questionable includes

namespace eda::gate::library {

/**
 * \brief Liberty file format parser based on ReadCells library.
 */
class ReadCellsParser: public CellSourceFileParserIface {
public:
  ReadCellsParser(std::string filename);

  std::vector<StandardCell> extractCells() override;
  std::vector<LutTemplate> extractTemplates() override;
private:
  bool loadCombCell(StandardCell& standardCell,
                    const readcells::AttributeList &rcCell);
  void setCellPins(StandardCell& standardCell,
                   const readcells::AttributeList &rcCell);
  void setCellProperties(StandardCell& standardCell,
                         const readcells::AttributeList &rcCell);
  double getArea(const readcells::AttributeList &rcCell);
  double getLeakagePower(const readcells::AttributeList &rcCell);

  bool isCombCell(const readcells::AttributeList &rcCell);
  std::vector<std::string> getInputPinNames(
    const readcells::AttributeList &rcCell);
  std::vector<std::string> getOutputPinNames(
    const readcells::AttributeList &rcCell);
  const readcells::AttributeList *getOutputPin(
    const readcells::AttributeList &rcCell, uint number = 0);
  const readcells::Expr *getExprFunction(
    const readcells::AttributeList &rcCell, uint number = 0);


  readcells::TokenParser tokParser_;
  readcells::Library library_;
};

} // namespace eda::gate::library