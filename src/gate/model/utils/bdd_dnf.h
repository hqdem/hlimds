//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#ifndef BDD_DNF_H
#define BDD_DNF_H

#include "util/logging.h"

#include "cuddObj.hh"
#include "kitty/kitty.hpp"

#include <vector>

namespace eda::gate::model::utils {

/**
 * \brief Implements functions for transferring BDD to a DNF.
*/
class BddToDnf {
public:

  using Paths = std::vector<std::vector<std::pair<int, bool>>>;
  using Path  = std::vector<std::pair<int, bool>>;

  //===--------------------------------------------------------------------===//
  // Convenience Methods
  //===--------------------------------------------------------------------===//

  /**
  * Builds DNF from BDD recursively traversing the tree.
  * @param bdd BDD tree from which to build DNF.
  * @return Return DNF.
  */
  static std::vector<kitty::cube> getDnf(BDD bdd);

};

} // namespace eda::gate::model::utils

#endif // BDD_DNF_H
