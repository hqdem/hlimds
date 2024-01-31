//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"

namespace eda::gate::debugger::options {

enum LecType {
  BDD,
  FRAIG,
  RND,
  SAT,
};

} // namespace eda::gate::debugger::options

namespace eda::gate::debugger {
using GateId = eda::gate::model::Gate::Id;
using GateIdMap = std::unordered_map<GateId, GateId>;
using GNet = eda::gate::model::GNet;
using LecType = eda::gate::debugger::options::LecType;

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
   * @param counterEx Counterexample for non-equivalent combinational nets.
   */
  CheckerResult(CheckerStatus status, std::vector<bool> counterEx) {
    assert(status == CheckerStatus::NOTEQUAL);
    this->status = status;
    this->counterEx = counterEx;
  }

  /**
   * @copydoc CheckerResult::CheckerResult(CheckerStatus)
   * @param counterExVal Counterexample for non-equivalent combinational nets.
   * @param inputSizeSum Sum of inputs sizes (in bits).
   */
  CheckerResult(CheckerStatus status, size_t counterExVal, size_t inputSizeSum) {
    assert(status == CheckerStatus::NOTEQUAL);
    this->status = status;
    this->counterEx.resize(inputSizeSum);
    for (size_t i = 0; i < inputSizeSum; i++) {
      this->counterEx[i] = (counterExVal >> i) & 1;
    }
  }

  /// Checks if the status is error.
  bool isError() {
    return status == CheckerStatus::ERROR;
  }

  /// Checks if the status is unknown.
  bool isUnknown() {
    return status == CheckerStatus::UNKNOWN;
  }

  /// Checks if the status is equivalence.
  bool equal() {
    return status == CheckerStatus::EQUAL;
  }

  /// Checks if the status is non-equivalence.
  bool notEqual() {
    return status == CheckerStatus::NOTEQUAL;
  }

  /// Returns a counter example if the status is non-equivalence.
  std::vector<bool> getCounterExample() {
    assert(status == CheckerStatus::NOTEQUAL);
    return this->counterEx;
  }

private:
  std::vector<bool> counterEx = {};
};

/// Basic class for equivalence checkers.
class BaseChecker {
public:
  /**
   * \brief Checks the equivalence of the given nets.
   * @param lhs First net.
   * @param rhs Second net.
   * @param gmap Gate-to-gate mapping between corresponding PI/PO of two nets.
   * @return The result of the check.
   */
  virtual CheckerResult equivalent(const GNet &lhs,
                                   const GNet &rhs,
                                   const GateIdMap &gmap) const = 0;

//===----------------------------------------------------------------------===//
// Utilities
//===----------------------------------------------------------------------===//

  /**
   * \brief Utility. Checks if the given nets are equivalent.
   * @param lhs First net.
   * @param rhs Second net.
   * @param gmap Gate-to-gate mapping between corresponding PI/PO of two nets.
   * @return true if the nets are equivalent, false otherwise.
   */
  bool areEqual(const GNet &lhs,
                const GNet &rhs,
                const GateIdMap &gmap);
  virtual ~BaseChecker() = 0;
};

BaseChecker &getChecker(LecType lec);

} // namespace eda::gate::debugger
