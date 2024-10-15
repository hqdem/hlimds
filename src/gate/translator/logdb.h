//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/npndb.h"

#include <fstream>
#include <iostream>
#include <string>

namespace eda::gate::translator {

/**
 * @brief Translates file with logic database into NpnDatabase class.
 */
class LogDbTranslator final {
public:
  optimizer::NpnDatabase translate(std::istream &in) const;

  optimizer::NpnDatabase translate(const std::string &filename) const {
    std::ifstream in(filename);
    return translate(in);
  }
};

} // namespace eda::gate::translator
