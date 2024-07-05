//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/get_dbstat.h"

namespace eda::gate::optimizer {

bool getDbStat(std::ostream &out, const NPNDBConfig &npndbConfig) {

  // processing the exclusion of wrong path to db
  if (!std::filesystem::exists(npndbConfig.dbPath)) {
    out << "Wrong DB path" << std::endl;
    return 1;
  }

  NPNDatabase currDatabase;

  // processing the exclusion of wrong type of db
  try {
    currDatabase = NPNDatabase::importFrom(npndbConfig.dbPath);
  } catch (const std::runtime_error &e) {
    out << "Wrong format of DB" << std::endl;
    return 1;
  }

  // processing the exclusion of wrong number of inputs
  // (exact_npn_canonization() limit)
  if (npndbConfig.ttSize < MIN_IN_SIZE || npndbConfig.ttSize > MAX_IN_SIZE) {
    out << "An incalculable size: " << unsigned(npndbConfig.ttSize)
        << "\nSize of inputs should be from " << unsigned(MIN_IN_SIZE) << " to "
        << unsigned(MAX_IN_SIZE) << std::endl;
    return 1;
  }

  kitty::dynamic_truth_table tt(npndbConfig.ttSize);

  for (const auto &binLine : npndbConfig.binLines) {
    // processing the exclusion of wrong length of binary line
    if (binLine.size() == pow(2, npndbConfig.ttSize)) {
      auto begin = binLine.begin();
      auto end = binLine.end();
      // processing the exclusion of bad-formatted line
      if (size_t(std::count(begin, end, '0') + std::count(begin, end, '1')) ==
          binLine.size()) {
        // adding an output line to tt for each binLine
        create_from_binary_string(tt, binLine);
      } else {
        out << "The line should be binary" << std::endl;
        return 1;
      }
    } else {
      out << "Wrong length of the values, your line size is " << binLine.size()
          << " correct size is " << pow(2, npndbConfig.ttSize) << std::endl;
      return 1;
    }
  }

  // processing the exclusion of wrong outType
  if (npndbConfig.outType != DOT && npndbConfig.outType != INFO &&
      npndbConfig.outType != BOTH) {
    out << "Wrong type of output, correct are (DOT / INFO / BOTH)" << std::endl;
    return 1;
  }

  // processing the exclusion of equivalent scheme absence
  try {
    switch (npndbConfig.outType) {
    case INFO:
      currDatabase.printInfo(out, tt);
      break;

    case BOTH:
      currDatabase.printInfo(out, tt);

    case DOT:
      if (!npndbConfig.outName.empty()) {
        if (npndbConfig.outName.size() < 4 ||
            npndbConfig.outName.substr(npndbConfig.outName.size() - 4) !=
                ".dot") {
          currDatabase.printDotFile(tt, npndbConfig.outName + ".dot",
                                    npndbConfig.binLines[0]);
        } else {
            currDatabase.printDotFile(tt, npndbConfig.outName,
                                      npndbConfig.binLines[0]);
        }
      } else {
        currDatabase.printDot(out, tt, npndbConfig.binLines[0]);
      }
      break;
    }
  } catch (const std::runtime_error &e) {
    out << "No equivalent scheme has been found" << std::endl;
    return 1;
  }

  return 0;
}

} // namespace eda::gate::optimizer