//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"

namespace eda::gate::debugger::options {

enum LecType {
  BDD,
  DEFAULT,
  RND,
};

} // namespace eda::gate::debugger::options

namespace eda::gate::debugger {
using Gate = eda::gate::model::Gate;
using GateId = eda::gate::model::Gate::Id;
using GateIdMap = std::unordered_map<GateId, GateId>;
using GNet = eda::gate::model::GNet;
using LecType = eda::gate::debugger::options::LecType;

// Equivalence checkers return value
// EQUAL returns if there exhaustive check and nets are equal
// UNKNOWN returns if there NO exhaustive check and the result is undefined
// NOTEQUAL returns if nets are not equal
// ERROR returns if invalid arguments were given
struct CheckerResult {
  enum Results {
    ERROR = -2,
    UNKNOWN = -1,
    EQUAL = 0,
    NOTEQUAL = 1,
  };
  Results result;
  CheckerResult(Results result) : result(result) {}

  bool isError() {
    return result == Results::ERROR;
  }

  bool isUnknown() {
    return result == Results::UNKNOWN;
  }

  bool equal() {
    return result == Results::EQUAL;
  }

  bool notEqual() {
    return result == Results::NOTEQUAL;
  }
};

class BaseChecker {
public:
  /**
 *  \brief Checks if the given nets are equal.
 *  @param lhs First net.
 *  @param rhs Second net.
 *  @param gmap Gate-to-gate mapping between nets.
 *  @return true if the nets are equal, false otherwise.
 */
  bool areEqual(GNet &lhs,
                GNet &rhs,
                GateIdMap &gmap);
  /**
 *  \brief Checks the equivalence of the given nets.
 *  @param lhs First net.
 *  @param rhs Second net.
 *  @param gmap Gate-to-gate mapping between nets.
 *  @return The result of the check.
 */
  virtual CheckerResult equivalent(GNet &lhs,
                                   GNet &rhs,
                                   GateIdMap &gmap) = 0;
  virtual ~BaseChecker() = 0;
};

BaseChecker &getChecker(LecType lec);

} // namespace eda::gate::debugger
