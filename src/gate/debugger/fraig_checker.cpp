//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/fraig_checker.h"
#include "gate/debugger/sat_checker.h"
#include "gate/optimizer/cone_builder.h"

#include <limits>
#include <map>
#include <random>
#include <set>

namespace eda::gate::debugger {

using Cone = optimizer::ConeBuilder::Cone;
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
    const auto &miter = model::Subnet::get(miterBuilder.make());
    const uint16_t nIn = miter.getInNum();

    // Simulation
    simulator::Simulator simulator(miter);
    if (storageCount) {
      netSimulation(simulator, miter.getInNum(), counterExStorage);
      std::fill(counterExStorage.begin(),
                counterExStorage.end(),
                std::bitset<64>());
      storageCount = 0;
    } else {
      netSimulation(simulator, nIn);
    }
    
    // Initial classes
    std::map<uint32_t, uint64_t> idxToValue;
    const uint16_t nOut = miter.getOutNum();
    const auto entries = miter.getEntries();
    for (size_t i = nIn; i < (entries.size() - nOut); i++) {
      idxToValue.emplace(i, simulator.getValue(i));
      i += entries[i].cell.more;
    }

    std::unordered_map<uint64_t, std::set<uint32_t>> eqClassToIdx;
    model::SubnetBuilder::MergeMap toBeMerged;
    std::unordered_set<uint32_t> checked;

    for (auto const &[cell, eqClass] : idxToValue) {
      if (compareCount > compareLimit) {
        break;
      }
      if (eqClassToIdx.find(eqClass) != eqClassToIdx.end()) {
        optimizer::ConeBuilder coneBuilder(&miter);
        for (auto idx : eqClassToIdx[eqClass]) {
          compareCount += 1;
          if (idx == cell || checked.find(idx) != checked.end()) {
            continue;    
          }
          const auto &cone1 = coneBuilder.getMaxCone(cell);
          const auto &cone2 = coneBuilder.getMaxCone(idx);
          const auto &coneSubnet1 = model::Subnet::get(cone1.subnetID);
          const auto &coneSubnet2 = model::Subnet::get(cone2.subnetID);
          CellToCell map = {};
          size_t coneInNum1 = coneSubnet1.getInNum();
          size_t coneInNum2 = coneSubnet2.getInNum();
          if (coneInNum1 != coneInNum2) {
            eqClassToIdx[eqClass].insert(cell);
            continue;
          }
          bool inputsFlag = false;
          for (size_t i = 0; i < coneSubnet1.getInNum(); ++i) {
            map[i] = i;
             if (cone1.inOutToOrig.at(i) != cone2.inOutToOrig.at(i)) {
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
          CheckerResult res = getChecker(SAT).areEquivalent(cone1.subnetID,
                                                            cone2.subnetID,
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
            std::vector<bool> proof(miter.getInNum(), false);
            for (uint16_t i = 0; i < counterExSize; i++) {
              proof[cone1.coneEntryToOrig.at(i)] = counterEx[i];
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
    }

    if (toBeMerged.empty()) {
      break;
    }
    miterBuilder.mergeCells(toBeMerged);
  }

  return getChecker(SAT).isSat(miterBuilder.make());
}

} // namespace eda::gate::debugger
