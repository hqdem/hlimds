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
// EQUAL returns if there is exhaustive check and nets are equal
// UNKNOWN returns if there is NO exhaustive check and the result is undefined
// NOTEQUAL returns if nets are not equal
// ERROR returns if an error occured 
struct CheckerResult {
  enum Result {
    ERROR = -2,
    UNKNOWN = -1,
    EQUAL = 0,
    NOTEQUAL = 1,
  };

  Result result;

  CheckerResult(Result result) : result(result) {}

  CheckerResult(Result result, std::vector<bool> counterEx) {
    assert(result == Result::NOTEQUAL);
    this->result = result;
    this->counterEx = counterEx;
  }

  CheckerResult(Result result, size_t counterExVal, size_t valSize) {
    assert(result == Result::NOTEQUAL);
    this->result = result;
    this->counterEx.resize(valSize);
    for (size_t i = 0; i < valSize; i++) {
      this->counterEx[i] = (counterExVal >> i) & 1;
    }
  }

  bool isError() {
    return result == Result::ERROR;
  }

  bool isUnknown() {
    return result == Result::UNKNOWN;
  }

  bool equal() {
    return result == Result::EQUAL;
  }

  bool notEqual() {
    return result == Result::NOTEQUAL;
  }

  std::vector<bool> getCounterExample() {
    assert(result == Result::NOTEQUAL);
    return this->counterEx;
  }

private:
  std::vector<bool> counterEx = {};
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
