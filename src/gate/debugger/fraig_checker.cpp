//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/fraig_checker.h"
#include "gate/debugger/sat_checker.h"
#include "gate/optimizer2/cone_builder.h"

#include <limits>
#include <map>
#include <random>
#include <set>

namespace eda::gate::debugger {

using Cone = optimizer2::ConeBuilder::Cone;
using options::SAT;
using UniformIntDistribution = std::uniform_int_distribution<uint64_t>;

// TODO: Utilize singleton
struct CellDescriptor {
  CellDescriptor(uint64_t eqClass) : eqClass(eqClass) {}
  
  void setEqClass(uint64_t eqClass) {
    this->eqClass = eqClass;
  }

  void setSingleton(bool singleton) {
    this->singleton = singleton;
  }

  uint64_t getEqClass() const {
    return eqClass;
  }

  bool isSingleton() const {
    return singleton;
  }

private:
  bool singleton = false;    
  uint64_t eqClass;
};

void FraigChecker::netSimulation(simulator2::Simulator &simulator,
                                  const uint16_t &nIn,
                                  const CounterEx &counterEx) {
  simulator2::Simulator::DataVector values(nIn);
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

CheckerResult FraigChecker::isSat(const SubnetID id) const {
  SubnetBuilder miterBuilder(id);
  const Subnet &miter = Subnet::get(id);
  uint16_t storageCount = 0;
  CounterEx counterExStorage(miter.getInNum());

  while (true) {
    const Subnet &miter = Subnet::get(miterBuilder.make());
    const uint16_t nIn = miter.getInNum();

    // Simulation
    simulator2::Simulator simulator(miter);
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
    std::map<uint32_t, CellDescriptor> idxToDescr;
    const uint16_t nOut = miter.getOutNum();
    const auto entries = miter.getEntries();
    for (size_t i = nIn; i < (entries.size() - nOut); i++) {
      CellDescriptor desc(simulator.getValue(i));
      idxToDescr.emplace(i, std::move(desc));
      i += entries[i].cell.more;
    }

    std::unordered_map<uint64_t, std::set<uint32_t>> eqClassToIdx;
    SubnetBuilder::MergeMap toBeMerged;
    std::unordered_set<uint32_t> checked;

    for (auto const &[cell, descr] : idxToDescr) {
      if (descr.isSingleton()) {
        continue;
      }
      const uint64_t eqClass = descr.getEqClass();
      if (eqClassToIdx.find(eqClass) != eqClassToIdx.end()) {
        optimizer2::ConeBuilder coneBuilder(&miter);
        for (auto idx : eqClassToIdx[eqClass]) {
          if (idx == cell || checked.find(idx) != checked.end()) {
            continue;    
          }
          const Cone &cone1 = coneBuilder.getMaxCone(cell);
          const Cone &cone2 = coneBuilder.getMaxCone(idx);
          const Subnet &coneSubnet1 = Subnet::get(cone1.subnetID);
          const Subnet &coneSubnet2 = Subnet::get(cone2.subnetID);
          CellToCell map = {};
          size_t coneInNum1 = coneSubnet1.getInNum();
          size_t coneInNum2 = coneSubnet2.getInNum();
          if (coneInNum1 != coneInNum2) {
            eqClassToIdx[eqClass].insert(cell);   
            continue;
          }
          for (size_t i = 0; i < coneSubnet1.getInNum(); ++i) {
            map[i] = i;
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

    for (auto eqClass : eqClassToIdx) {
      if (eqClass.second.size() == 1) {
        idxToDescr.at(*eqClass.second.begin()).setSingleton(true); 
      }
    }
    if (toBeMerged.empty()) {
      break;
    }
    miterBuilder.mergeCells(toBeMerged);
  }

  return static_cast<SatChecker&>(getChecker(SAT)).isSat(miterBuilder.make());
}

} // namespace eda::gate::debugger
