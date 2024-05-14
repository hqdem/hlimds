//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"

namespace eda::gate::debugger2::options {

enum LecType {
  BDD,
  FRAIG,
  RND,
  SAT,
};

} // namespace eda::gate::debugger2::options

namespace eda::gate::debugger2 {
using CellToCell = std::unordered_map<size_t, size_t>;
using LecType = eda::gate::debugger2::options::LecType;
using Subnet = model::Subnet;
using SubnetBuilder = model::SubnetBuilder;
using SubnetID = model::SubnetID;

/// Equivalence checking result.
struct CheckerResult {
  /**
   * Equivalence checking statuses.
   * ERROR if an internal error occurred.
   * EQUAL if nets are equivalent.
   * NOTEQUAL if nets are not equivalent.
   * UNKNOWN if the checking result is undefined.
   */
  enum CheckerStatus {
    ERROR = -2,
    UNKNOWN = -1,
    EQUAL = 0,
    NOTEQUAL = 1,
  };

  /// Equivalence checking status.
  CheckerStatus status;

  /**
   * \brief Equivalence checking result.
   * @param status Status of the occurred equivalence check.
   */
  CheckerResult(CheckerStatus status) : status(status) {}

  /**
   * @copydoc CheckerResult::CheckerResult(CheckerStatus)
   * @param counterEx Input values on which the nets are unequal.
   */
  CheckerResult(CheckerStatus status, std::vector<bool> counterEx) {
    assert(status == CheckerStatus::NOTEQUAL);
    this->status = status;
    this->counterEx = counterEx;
  }

  /// Checks if the status is error.
  bool isError() const {
    return status == CheckerStatus::ERROR;
  }

  /// Checks if the status is unknown.
  bool isUnknown() const {
    return status == CheckerStatus::UNKNOWN;
  }

  /// Checks if the status is equivalence.
  bool equal() const {
    return status == CheckerStatus::EQUAL;
  }

  /// Checks if the status is non-equivalence.
  bool notEqual() const {
    return status == CheckerStatus::NOTEQUAL;
  }

  /// Returns a counter example if the status is non-equivalence.
  std::vector<bool> getCounterExample() const {
    assert(status == CheckerStatus::NOTEQUAL);
    return this->counterEx;
  }

private:
  std::vector<bool> counterEx = {};
};

/// Basic class for equivalence checkers.
class BaseChecker2 {
public:

  /**
   * \brief Constructs a miter for the specified nets.
   * @param builder Builder, in which a miter is constructed.
   * @param lhs First net.
   * @param rhs Second net.
   * @param gmap Gate-to-gate mapping between corresponding PI/PO of two nets.
   */
  static void miter2(SubnetBuilder &builder,
                     const SubnetID lhs,
                     const SubnetID rhs,
                     const CellToCell &gmap);
  /**
   * \brief Checks if the given single-output net is satisfiable.
   * @param id The net.
   * @return The result of the check.
   */
  virtual CheckerResult isSat(const SubnetID id) const = 0;

  /**
   * \brief Checks the equivalence of the given nets.
   * @param lhs First net.
   * @param rhs Second net.
   * @param gmap Gate-to-gate mapping between corresponding PI/PO of two nets.
   * @return The result of the check.
   */
  virtual CheckerResult areEquivalent(const SubnetID lhs,
                                      const SubnetID rhs,
                                      const CellToCell &gmap) const final {
    SubnetBuilder builder;
    miter2(builder, lhs, rhs, gmap);
    return isSat(builder.make());
  }

  virtual ~BaseChecker2() = 0;
};

BaseChecker2 &getChecker(LecType lec);

} // namespace eda::gate::debugger2
