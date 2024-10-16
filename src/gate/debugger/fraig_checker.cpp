//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/fraig_checker.h"
#include "gate/debugger/sat_checker.h"

#include <limits>
#include <map>
#include <random>
#include <set>

namespace eda::gate::debugger {

using options::SAT;
using UniformIntDistribution = std::uniform_int_distribution<uint64_t>;

struct EqPointResult {
  /**
   * Equivalence checking point statuses.
   * UNKNOWN if the result is undefined.
   * NOTEQUAL if the points are not equivalent.
   * EQUAL if the points are equivalent.
   */
  enum EqPointStatus {
    UNKNOWN = -2,
    NOTEQUAL = -1,
    EQUAL = 0,
  };

  /// Equivalence checking status.
  EqPointStatus status;

  /**
   * @brief Point equivalence checking result.
   * @param status Status of the occurred check.
   */
  EqPointResult(EqPointStatus status): status(status) {}

  /**
   * @copydoc EqPointResult::EqPointResult(EqPointStatus)
   * @param counterEx Input values on which the nets are unequal.
   * @param inputs Inputs mapping for a max cone.
   */
  EqPointResult(EqPointStatus status,
                        const std::vector<bool> &counterExample,
                        const model::Subnet::LinkList &inputs) {
    assert(status == EqPointStatus::NOTEQUAL);
    this->status = status;
    this->counterExample = counterExample;
    this->inputs = inputs;
    this->counterExampleFlag = true;
  }

  void fillStorage(SimValuesStorage &storage,
                   uint16_t &storageCount,
                   const uint16_t nIn) {
  if (storageCount >= FraigChecker::simLimit ||
      !counterExampleFlag) {
    return;
  }
  auto counterExampleSize = counterExample.size();

  UniformIntDistribution distribution(0, counterExampleSize);
  //TODO: Replace with general generator.
  std::mt19937 generator(std::random_device{}());
  uint16_t index = distribution(generator);
  counterExample[index].flip();
  std::vector<bool> proof(nIn, false);
  for (uint16_t i = 0; i < counterExampleSize; i++) {
    proof[inputs[i].idx] = counterExample[i];
  }
  for (size_t i = 0; i < proof.size(); i++) {
    storage[i].set(storageCount, proof[i]);
  }
  storageCount += 1;
}

private:
  bool counterExampleFlag = false;
  std::vector<bool> counterExample{};
  model::Subnet::LinkList inputs{};
};

EqPointResult check(const uint32_t point1,
                    const uint32_t point2,
                    const std::shared_ptr<model::SubnetBuilder> &builder) {

  model::SubnetView cone1(builder, point1);

  model::SubnetView cone2(builder, point2);

  const auto cone1InNum = cone1.getInNum();
  const auto cone2InNum = cone2.getInNum();

  if (cone1InNum != cone2InNum) {
    return EqPointResult::NOTEQUAL;
  }
  auto inputs1 = cone1.getInputs();
  auto inputs2 = cone2.getInputs();
  std::unordered_map<uint32_t, uint32_t> inputToPos;
  for (size_t i = 0; i < inputs2.size(); i++) {
    inputToPos[inputs2[i].idx] = i;
  }

  for (size_t i = 0; i < cone1InNum; ++i) {
    try {
      inputToPos.at(inputs1[i].idx);
    }
    catch (const std::out_of_range &e) {
      return EqPointResult::NOTEQUAL;
    }
  }
  // FIXME: Returns unknown.
  CheckerResult res = BaseChecker::getChecker(SAT).areEquivalent(cone1, cone2);

  if (res.notEqual()) {
    return EqPointResult(EqPointResult::NOTEQUAL,
                                 res.getCounterExample(),
                                 cone1.getInputs());
  }
  if (res.equal()) {
    return EqPointResult::EQUAL;
  }
  return EqPointResult::UNKNOWN;
}

void simulate(simulator::Simulator &simulator,
              const uint16_t nIn) {
  simulator::Simulator::DataVector values(nIn);
  //TODO: Replace with general generator.
  std::mt19937 generator;
  UniformIntDistribution distrib(0, std::numeric_limits<uint64_t>::max());
  for (size_t k = 0; k < nIn; ++k) {
    values[k] = distrib(generator);
  }
  simulator.simulate(values);
}

void simulate(simulator::Simulator &simulator,
              const uint16_t nIn,
              SimValuesStorage &storage) {
  simulator::Simulator::DataVector values(nIn);
  for (size_t k = 0; k < nIn; ++k) {
    values[k] = storage[k].to_ullong();
  }
  simulator.simulate(values);
  std::fill(storage.begin(), storage.end(), std::bitset<64>());
}

CheckerResult FraigChecker::isSat(const model::Subnet &subnet) const {
  auto miterBuilderPtr = std::make_shared<model::SubnetBuilder>(subnet);
  auto &miterBuilder = *miterBuilderPtr;
  uint16_t storageCount = 0;
  SimValuesStorage storage(subnet.getInNum());

  while (true) {
    size_t compareCount = 0;
    const uint16_t nIn = miterBuilder.getInNum();

    // Simulation
    simulator::Simulator simulator(miterBuilderPtr);
    if (storageCount) {
      simulate(simulator, nIn, storage);
      storageCount = 0;
    } else {
      simulate(simulator, nIn);
    }

    std::unordered_map<uint64_t, std::set<uint32_t>> eqClassToIdx;
    model::SubnetBuilder::MergeMap toBeMerged;
    std::unordered_set<uint32_t> checked;

    for (auto it = miterBuilder.begin();
              it != miterBuilder.end();
              it.nextCell()) {
      const auto &cell = miterBuilder.getCell(*it);
      if (cell.isOut() || cell.isIn()) {
        continue;
      }
      uint64_t const cellIdx1 = *it;
      uint64_t const eqClass = simulator.getValue(cellIdx1);
      if (compareCount > compareLimit) {
        break;
      }
      if (eqClassToIdx.find(eqClass) != eqClassToIdx.end()) {
        for (auto cellIdx2 : eqClassToIdx[eqClass]) {
          if (cellIdx1 == cellIdx2 || checked.find(cellIdx2) != checked.end()) {
            continue;
          }
          compareCount += 1;
          auto res = check(cellIdx1, cellIdx2, miterBuilderPtr);
          res.fillStorage(storage, storageCount, nIn);
          if (res.status == EqPointResult::EQUAL) {
            toBeMerged[cellIdx2].insert(cellIdx1);
            checked.insert(cellIdx1);
          } else {
            eqClassToIdx[eqClass].insert(cellIdx1);
          }
        }
      } else {
        eqClassToIdx[eqClass].insert(cellIdx1);
      }
    } // for idx-to-val

    if (toBeMerged.empty()) {
      break;
    }
    miterBuilder.mergeCells(toBeMerged);
  }
  // FIXME: Creates a subnet.
  return getChecker(SAT).isSat(miterBuilder);
}

} // namespace eda::gate::debugger
