//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/bdd_checker.h"
#include "gate/debugger/checker.h"
#include "gate/debugger/miter.h"
#include "gate/debugger/rnd_checker.h"
#include "gate/model/gnet_test.h"
#include "gate/parser/parser_test.h"
#include "gate/premapper/mapper/mapper_test.h"
#include "rtl/parser/ril/parser.h"
#include "util/logging.h"

#include "gtest/gtest.h"

#include <filesystem>

namespace eda::gate::debugger {

using Gate = eda::gate::model::Gate;
using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
using GNet = eda::gate::model::GNet;
using PreBasis = eda::gate::premapper::PreBasis;

/**
 *  \brief Checks equivalence of the parsed net and the premapped net.
 *  @param fileName Name of the source file.
 *  @param subPath Relative path to the file.
 *  @param checker LEC type.
 *  @param basis Premapper basis.
 *  @return The result of the check.
 */
CheckerResult fileLecTest(const std::string &fileName,
                          BaseChecker &checker,
                          PreBasis basis,
                          const std::string &subPath = "");

Checker::Hints checkerTestHints(unsigned N,
                                const GNet &lhs,
                                const Gate::SignalList &lhsInputs,
                                Gate::Id lhsOutputId,
                                const GNet &rhs,
                                const Gate::SignalList &rhsInputs,
                                Gate::Id rhsOutputId);

bool checkEquivTest(unsigned N,
                    const GNet &lhs,
                    const Gate::SignalList &lhsInputs,
                    Gate::Id lhsOutputId,
                    const GNet &rhs,
                    const Gate::SignalList &rhsInputs,
                    Gate::Id rhsOutputId);

bool checkEquivMiterTest(unsigned N,
                         GNet &lhs,
                         const Gate::SignalList &lhsInputs,
                         Gate::Id lhsOutputId,
                         GNet &rhs,
                         const Gate::SignalList &rhsInputs,
                         Gate::Id rhsOutputId);

bool checkNorNorTest(unsigned N);

bool checkNorAndnTest(unsigned N);

bool checkNorAndTest(unsigned N);

} // namespace eda::gate::debugger
