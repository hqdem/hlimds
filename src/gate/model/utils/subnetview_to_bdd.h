//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#ifndef SUBNETVIEW_TO_BDD_H
#define SUBNETVIEW_TO_BDD_H

#include "gate/model/subnetview.h"

#include "cuddObj.hh"

#include <functional>
#include <unordered_map>

namespace eda::gate::model::utils {

//===--------------------------------------------------------------------===//
// Types
//===--------------------------------------------------------------------===//

using BddList = std::vector<BDD>;
using BddMap = std::unordered_map<EntryID, BDD>;

/**
* Constructs a BDD for each output in the SubnetView.
* @param sv Link to current SubnetView.
* @param cudd Link to Cudd.
* @return resulting Forest of BDD.
*/
BddList convertBdd(const SubnetView &sv, const Cudd &cudd);

} // namespace eda::gate::model::utils

#endif // SUBNETVIEW_TO_BDD_H
