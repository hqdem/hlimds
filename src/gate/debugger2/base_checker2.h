//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

using namespace eda::gate::model;
namespace eda::gate::debugger2::options {

enum LecType {
  RND,
};

} // namespace eda::gate::debugger2::options

namespace eda::gate::debugger2 {
using CellToCell = std::unordered_map<size_t, size_t>;
using LecType = eda::gate::debugger2::options::LecType;

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

  CheckerResult(Result result, std::vector<uint64_t> counterEx) {
    assert(result == Result::NOTEQUAL);
    this->result = result;
    this->counterEx = counterEx;
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

  std::vector<uint64_t> getCounterExample() {
    assert(result == Result::NOTEQUAL);
    return this->counterEx;
  }

private:
  std::vector<uint64_t> counterEx = {};
};

class BaseChecker2 {
public:
  /**
 *  \brief Checks if the given nets are equal.
 *  @param lhs First net.
 *  @param rhs Second net.
 *  @param gmap Gate-to-gate mapping between nets.
 *  @return true if the nets are equal, false otherwise.
 */
  bool areEqual(Subnet &lhs,
                Subnet &rhs,
                CellToCell &gmap);
  /**
 *  \brief Checks the equivalence of the given nets.
 *  @param lhs First net.
 *  @param rhs Second net.
 *  @param gmap Gate-to-gate mapping between nets.
 *  @return The result of the check.
 */
  virtual CheckerResult equivalent(Subnet &lhs,
                                   Subnet &rhs,
                                   CellToCell &gmap) = 0;
  virtual ~BaseChecker2() = 0;
};

BaseChecker2 &getChecker(LecType lec);

} // namespace eda::gate::debugger2
