//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/tech_mapper/library/cell.h"
#include "gate/model/gnet_test.h"
#include "gate/optimizer/rwdatabase.h"

using namespace eda::gate::optimizer;

void initializeLibraryRwDatabase(std::vector<Cell*> &cells, SQLiteRWDatabase *arwdb);