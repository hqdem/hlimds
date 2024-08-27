//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/design.h"
#include "gate/model/subnet.h"
#include "gate/model/subnetview.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace eda::gate::debugger::options {

enum LecType {
  BDD,
  FRAIG,
  RND,
  SAT
};

} // namespace eda::gate::debugger::options

namespace eda::gate::debugger {

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
   * @brief Equivalence checking result.
   * @param status Status of the occurred equivalence check.
   */
  CheckerResult(CheckerStatus status): status(status) {}

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
  std::vector<bool> counterEx{};
};

/// Basic class for equivalence checkers.
class BaseChecker {
public:
  using CellToCell = std::unordered_map<uint32_t, uint32_t>;
  using LecType = eda::gate::debugger::options::LecType;

  /// Returns the LEC checker.
  static BaseChecker &getChecker(const LecType lec);

  /**
   * @brief Constructs the miter for the specified subnets.
   * @param builder Builder for constructing the miter.
   * @param subnet1 First subnet.
   * @param subnet2 Second subnet.
   * @param mapping Mapping between the PI/PO of the specified subnets.
   */
  static void makeMiter(model::SubnetBuilder &builder,
                        const model::Subnet &subnet1,
                        const model::Subnet &subnet2,
                        const CellToCell &mapping);

  /**
   * @brief Constructs the miter for the specified subnets.
   * @param builder Builder for constructing the miter.
   * @param subnet1 First subnet.
   * @param subnet2 Second subnet.
   * @param mapping Mapping between the PI/PO of the specified subnets.
   */
  static void makeMiter(model::SubnetBuilder &builder,
                        const model::SubnetID subnetID1,
                        const model::SubnetID subnetID2,
                        const CellToCell &mapping) {
    const auto &subnet1 = model::Subnet::get(subnetID1);
    const auto &subnet2 = model::Subnet::get(subnetID2);
    makeMiter(builder, subnet1, subnet2, mapping);
  }

  /**
   * @brief Constructs the miter for the specified subnets.
   * @param builder Builder for constructing the miter.
   * @param subnet1 First subnet.
   * @param subnet2 Second subnet.
   */
  static void makeMiter(model::SubnetBuilder &builder,
                        const model::Subnet &subnet1,
                        const model::Subnet &subnet2);

  /**
   * @brief Constructs the miter for the specified subnets.
   * @param builder Builder for constructing the miter.
   * @param subnetID1 Identifier of the first subnet.
   * @param subnetID2 Identifier of the second subnet.
   */
  static void makeMiter(model::SubnetBuilder &builder,
                        const model::SubnetID subnetID1,
                        const model::SubnetID subnetID2) {
    const auto &subnet1 = model::Subnet::get(subnetID1);
    const auto &subnet2 = model::Subnet::get(subnetID2);
    makeMiter(builder, subnet1, subnet2);
  }

  /**
   * @brief Constructs the miter for the specified subnets.
   * @param builder Builder for constructing the miter.
   * @param builder1 Builder of the first subnet.
   * @param builder2 Builder of the second subnet.
   * @param mapping Mapping between the PI/PO of the specified subnets.
   */
  static void makeMiter(model::SubnetBuilder &builder,
                        const model::SubnetBuilder &builder1,
                        const model::SubnetBuilder &builder2,
                        const CellToCell &mapping);
  
  /**
   * @brief Constructs the miter for the specified subnets.
   * @param builder Builder for constructing the miter.
   * @param builder1 Builder of the first subnet.
   * @param builder2 Builder of the second subnet.
   */
  static void makeMiter(model::SubnetBuilder &builder,
                        const model::SubnetBuilder &builder1,
                        const model::SubnetBuilder &builder2);
  /**
   * @brief Checks if the given single-output subnet is satisfiable.
   * @param subnetID Subnet to checked.
   * @return Checking result.
   */
  virtual CheckerResult isSat(const model::Subnet &subnet) const = 0;

  /**
   * @brief Checks if the given single-output subnet is satisfiable.
   * @param subnetID Identifier of the subnet.
   * @return Checking result.
   */
  CheckerResult isSat(const model::SubnetID subnetID) const {
    return isSat(model::Subnet::get(subnetID));
  }

  /**
   * @brief Checks if the given subnet builder is satisfiable.
   * @param builder Builder of the subnet.
   * @return Checking result.
   */
  CheckerResult isSat(model::SubnetBuilder &builder) const {
    return isSat(builder.make());
  }

  /**
   * @brief Checks the equivalence of the given subnets.
   * @param subnet1 First subnet.
   * @param subnet2 Second subnet.
   * @param mapping Mapping between the PI/PO of the specified subnets.
   * @return Checking result.
   */
  CheckerResult areEquivalent(const model::Subnet &subnet1,
                              const model::Subnet &subnet2,
                              const CellToCell &mapping) const;

  /**
   * @brief Checks the equivalence of the given subnets.
   * @param subnetID1 Identifier of the first subnet.
   * @param subnetID2 Identifier of the second subnet.
   * @param mapping Mapping between the PI/PO of the specified subnets.
   * @return Checking result.
   */
  CheckerResult areEquivalent(const model::SubnetID subnetID1,
                              const model::SubnetID subnetID2,
                              const CellToCell &mapping) const {
    const auto &subnet1 = model::Subnet::get(subnetID1);
    const auto &subnet2 = model::Subnet::get(subnetID2);
    return areEquivalent(subnet1, subnet2, mapping);
  }

  /**
   * @brief Checks the equivalence of the given subnets.
   * @param subnet1 First subnet.
   * @param subnet2 Second subnet.
   * @return Checking result.
   */
  CheckerResult areEquivalent(const model::Subnet &subnet1,
                              const model::Subnet &subnet2) const;

  /**
   * @brief Checks the equivalence of the given subnets.
   * @param subnetID1 Identifier of the first subnet.
   * @param subnetID2 Identifier of the second subnet.
   * @return Checking result.
   */
  CheckerResult areEquivalent(const model::SubnetID subnetID1,
                              const model::SubnetID subnetID2) const {
    const auto &subnet1 = model::Subnet::get(subnetID1);
    const auto &subnet2 = model::Subnet::get(subnetID2);
    return areEquivalent(subnet1, subnet2);
  }

  /**
   * @brief Checks the equivalence of the given check points of the design.
   * @param point1 Identifier of the first subnet.
   * @param point2 Identifier of the second subnet.
   * @return Checking result.
   */
  CheckerResult areEquivalent(model::DesignBuilder &builder,
                              const std::string &point1,
                              const std::string &point2) const;

  /**
   * @brief Checks the equivalence of the given subnet views.
   * @param subnetView1 View of the first subnet.
   * @param subnetView2 View of the second subnet.
   * @return Checking result.
   */
  CheckerResult areEquivalent(const model::SubnetView &subnetView1,
                              const model::SubnetView &subnetView2) const;

  /**
   * @brief Checks the equivalence of the given subnet builders.
   * @param builder1 Builder of the first subnet.
   * @param builder2 Builder of the second subnet.
   * @param mapping Mapping between the PI/PO of the specified builders.
   * @return Checking result.
   */
  CheckerResult areEquivalent(const model::SubnetBuilder &builder1,
                              const model::SubnetBuilder &builder2,
                              const CellToCell &mapping) const;

  virtual ~BaseChecker() {}
};

} // namespace eda::gate::debugger
