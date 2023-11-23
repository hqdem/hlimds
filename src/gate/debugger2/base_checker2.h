//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/base_checker.h"
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
using CheckerResult = eda::gate::debugger::CheckerResult;

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
