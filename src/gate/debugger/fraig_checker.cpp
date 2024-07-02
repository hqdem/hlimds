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

void FraigChecker::netSimulation(simulator::Simulator &simulator,
                                 const uint16_t &nIn,
                                 const CounterEx &counterEx) {
  simulator::Simulator::DataVector values(nIn);
  if (counterEx.size()) {
    for (size_t k = 0; k < nIn; ++k) {
      values[k] = counterEx[k].to_ullong();
    }
  } else {
    std::mt19937 generator;
    UniformIntDistribution distrib(0, std::numeric_limits<uint64_t>::max());
    for (size_t k = 0; k < nIn; ++k) {
      values[k] = distrib(generator);
    }
  }
  simulator.simulate(values);
}

CheckerResult FraigChecker::isSat(const model::Subnet &subnet) const {
  model::SubnetBuilder miterBuilder(subnet);
  uint16_t storageCount = 0;
  CounterEx counterExStorage(subnet.getInNum());

  while (true) {
    size_t compareCount = 0;
    const uint16_t nIn = miterBuilder.getInNum();

    // Simulation
    simulator::Simulator simulator(miterBuilder);
    if (storageCount) {
      netSimulation(simulator, nIn, counterExStorage);
      std::fill(counterExStorage.begin(),
                counterExStorage.end(),
                std::bitset<64>());
      storageCount = 0;
    } else {
      netSimulation(simulator, nIn);
    }
    
    // Initial classes
    std::map<uint32_t, uint64_t> idxToValue;

    for (auto it = miterBuilder.begin(); it != miterBuilder.end(); it.nextCell()) {
      idxToValue.emplace(*it, simulator.getValue(*it));
    }

    std::unordered_map<uint64_t, std::set<uint32_t>> eqClassToIdx;
    model::SubnetBuilder::MergeMap toBeMerged;
    std::unordered_set<uint32_t> checked;

    for (auto const &[cell, eqClass] : idxToValue) {
      if (compareCount > compareLimit) {
        break;
      }
      if (eqClassToIdx.find(eqClass) != eqClassToIdx.end()) {
        for (auto idx : eqClassToIdx[eqClass]) {
          compareCount += 1;
          if (idx == cell || checked.find(idx) != checked.end()) {
            continue;    
          }

          model::SubnetView cone1(miterBuilder, cell);
          const auto coneSubnetID1 = cone1.getSubnet().make(); // FIXME: Do not create a subnet.
          const auto &coneSubnet1 = cone1.getSubnet().object();
          const auto coneInNum1 = coneSubnet1.getInNum();
          assert(coneInNum1 == cone1.getInNum());

          model::SubnetView cone2(miterBuilder, idx);
          const auto coneSubnetID2 = cone2.getSubnet().make(); // FIXME: Do not create a subnet.
          const auto &coneSubnet2 = cone2.getSubnet().object();
          const auto coneInNum2 = coneSubnet2.getInNum();
          assert(coneInNum2 == cone2.getInNum());

          CellToCell map = {};
          if (coneInNum1 != coneInNum2) {
            eqClassToIdx[eqClass].insert(cell);
            continue;
          }
          bool inputsFlag = false;
          for (size_t i = 0; i < coneInNum1; ++i) {
            map[i] = i; // FIXME: Order of inputs may by different.
            if (cone1.getIn(i) != cone2.getIn(i)) {
              inputsFlag = true;
              break;
            }
          }
          if (inputsFlag) {
            eqClassToIdx[eqClass].insert(cell);
            continue;
          }
          map[coneSubnet1.getEntries().size() - 1] =
          coneSubnet2.getEntries().size() - 1;
          CheckerResult res = getChecker(SAT).areEquivalent(coneSubnetID1, // FIXME: Use a builder.
                                                            coneSubnetID2, // FIXME: Use a builder.
                                                            map);
          if (res.equal()) {
            toBeMerged[idx].insert(cell);
            checked.insert(cell);   
          } else {
            eqClassToIdx[eqClass].insert(cell);
            if (storageCount >= simLimit) {
              continue;
            }
            std::vector<bool> counterEx = res.getCounterExample();
            size_t counterExSize = counterEx.size();

            UniformIntDistribution distribution(0, counterExSize);
            std::mt19937 generator(std::random_device{}());
            uint16_t index = distribution(generator);
            counterEx[index].flip();
            std::vector<bool> proof(nIn, false);
            for (uint16_t i = 0; i < counterExSize; i++) {
              proof[cone1.getIn(i) /*FIXME*/] = counterEx[i];
            }
            for (size_t i = 0; i < proof.size(); i++) {
              counterExStorage[i].set(storageCount, proof[i]);
            }
            storageCount += 1;
          }
        }
      } else {
        eqClassToIdx[eqClass].insert(cell);
      }
    } // for idx-to-val

    if (toBeMerged.empty()) {
      break;
    }
#if 0
    // FIXME: Uncomment.
    miterBuilder.mergeCells(toBeMerged);
#else
    // FIXME: Remove. 
    break;
#endif
  }

  return getChecker(SAT).isSat(miterBuilder.make()); // FIXME: Do not create a subnet.
}

} // namespace eda::gate::debugger
