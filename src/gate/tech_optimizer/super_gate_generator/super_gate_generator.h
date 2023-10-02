//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/celltype.h"
#include "gate/model2/subnet.h"

#include <vector>

using CellTypeID = eda::gate::model::CellTypeID;
using Subnet = eda::gate::model::Subnet;

namespace eda::gate::tech_optimizer {
/**
 * \brief .
 * \author <a href="mailto:dgaryaev@ispras.ru"></a>
 **/

class SuperGateGenerator {

public:
SuperGateGenerator(std::vector<const CellTypeID*> &library, 
    unsigned int maxSuperGatesInputs, unsigned int maxDepth);

std::vector<Subnet*> generate();

private:
void createSuperGate();
bool outOfCombinations();
void getNextComb();

std::vector<const CellTypeID*> library;
unsigned int maxSuperGatesInputs;
unsigned int maxDepth;

std::vector<std::vector<CellTypeID*>::iterator> inputsElem;
};

} // namespace eda::gate::tech_optimizer