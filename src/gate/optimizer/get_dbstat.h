//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/npndb.h"

#include "kitty/kitty.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace eda::gate::optimizer {

/** 
 * \brief Сonstants limit the size of inputs of the truth table. 
 * The limitation is due to the function `exact_npn_canonization()` 
 * from `./lib/kitty/include/kitty/npn.hpp`
 */
const uint8_t MIN_IN_SIZE = 1;
const uint8_t MAX_IN_SIZE = 6;

enum OutType { DOT, INFO, BOTH };

/**
 * \brief the structure is used to use the db stat function 
 * and pass all parameters in a single config variable.
 * \param dbPath the path to file with DB.
 * \param ttSize number of inputs of truth table.
 * \param outType type of the output (only DOT representation, 
 *  only information about elements or both in the same time)/
 * \param outName name of the file to save output.
 * \param binLines list of outputs of truth table. 
 *  Each line is a binary line for one output.
 */
struct NpnDbConfig {
  std::string dbPath;
  size_t ttSize;
  OutType outType;
  std::string outName;
  std::vector<std::string> binLines;
};

bool getDbStat(std::ostream &out, const NpnDbConfig &npndbConfig);

} // namespace eda::gate::optimizer
