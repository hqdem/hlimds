//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/checker.h"
#include "gate/debugger/bdd_checker.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/premapper/mapper/mapper_test.h"
#include "rtl/compiler/compiler.h"
#include "rtl/parser/ril/parser.h"

#include "gtest/gtest.h"

#include <filesystem>

namespace eda::gate::debugger {
using Gate = eda::gate::model::Gate;
using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
using GNet = eda::gate::model::GNet;
using PreBasis = eda::gate::premapper::PreBasis;

CheckerResult rilEquivalenceTest(const std::string &outSubPath,
                                 const std::string &fileName,
                                 BaseChecker &checker,
                                 PreBasis basis);
} // namespace eda::gate::debugger
