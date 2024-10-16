//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#ifndef BDD_H
#define BDD_H

#include "cuddObj.hh"

namespace eda::gate::model {

struct Bdd {
  ~Bdd() {
    //FIXME
    // delete cudd;
  }
  BDD diagram;
  Cudd *cudd;
};

} // namespace eda::gate::model

#endif // BDD_H
