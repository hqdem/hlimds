//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once
#include "gate/library/library_types.h"
#include <vector>

namespace eda::gate::library {

struct CSFProperties {
  std::string defaultWLM = "";
  WireLoadSelection WLSelection;
};

/**
 * \brief Parser interface for cell files
 */
class CellSourceFileParserIface {
public:
  //Methods to fill base types
  virtual std::vector<StandardCell> extractCells() = 0;
  virtual std::vector<WireLoadModel> extractWLMs() = 0;
  virtual std::vector<LutTemplate> extractTemplates() = 0;
  virtual CSFProperties extractProperties() = 0;
  virtual ~CellSourceFileParserIface() {};
};

} // namespace eda::gate::library