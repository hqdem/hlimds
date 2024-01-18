//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/fraig_checker.h"

namespace eda::gate::debugger {

EqClasses FraigChecker::initClasses(GNet *premapped) {
  auto compiled = makeCompiled(*premapped);
  EqClasses classes = {{}};
  for (auto *gate : premapped->gates()) {
    if (!(gate->isSource() || gate->isTarget())) {
      classes[0].push_back(gate->id());
    }
  }
  if (classes[0].size() <= 1) {
    classes.clear();
    return classes;  
  }
  size_t output;
  // Initial simulation & classes finding
  std::mt19937 generator;
  size_t nInputs = premapped->nSourceLinks();
  std::uniform_int_distribution<size_t> distribution(1, 1ULL << nInputs);
  for (size_t i = 0; i < initSimNum; i++) {
    size_t temp = distribution(generator);
    compiled.simulate(output, temp);
    size_t classesSize = classes.size();
    for (size_t i = 0; i < classesSize; i++) {
      splitClass(classes, classes[0], compiled);
    }
  }
  // Elaboration of classes until saturation (random values)
  saturation(compiled, classes, *premapped);
  return classes;
}

void FraigChecker::mergeConsts(std::vector<GateId> &consts,
                               GNet *premapped,
                               bool oneOrZero) {
  if (consts.empty()) {
    return;
  }
  GateId mergeConst;
  GateSymbol value;
  if (oneOrZero) {
    mergeConst = premapped->addOne();
    value = GateSymbol::ONE;
  } else {
    mergeConst = premapped->addZero();
    value = GateSymbol::ZERO;
  }

  premapped->sortTopologically();
  std::vector<GateId> cutNodes;
  cutNodes.reserve(premapped->nSourceLinks());
  for (auto link : premapped->sourceLinks()) {
    cutNodes.push_back(link.target);
  }
  Checker checker;
  for (auto gid : consts) {
    if (Gate::get(gid)->func() == value) {
      premapped->mergeGates(gid, mergeConst);
      continue;
    }
    BoundGNet bind = optimizer::extractCone(premapped, gid, cutNodes);
    GNet &cone = *(bind.net);
    if (cone.isEmpty()) {
      continue;
    }
    cone.sortTopologically();
    CheckerResult coneResult = checker.isEqualCombSatMiter(cone);
    if (coneResult.equal()) {
      premapped->mergeGates(gid, mergeConst);
    }
  }
  premapped->sortTopologically();
}

void FraigChecker::splitClass(EqClasses &eqClasses,
                              std::vector<GateId> &eqClass,
                              Compiled &comp) {
  auto partition_index = std::stable_partition(eqClass.begin(),
                                               eqClass.end(),
                                               [&comp](GateId curGid) {
                                                 return !comp.getValue(curGid);
                                               });
  std::vector<GateId> zeroClass, oneClass;
  zeroClass.assign(eqClass.begin(), partition_index);
  oneClass.assign(partition_index, eqClass.end());

  if (zeroClass.empty() || oneClass.empty() ) {
    eqClasses.push_back(eqClass);
    eqClasses.erase(eqClasses.begin());
    return;
  }
  if (zeroClass.size() > 1) {
    eqClasses.push_back(zeroClass);
  }
  if (oneClass.size() > 1) {
    eqClasses.push_back(oneClass);
  }
  eqClasses.erase(eqClasses.begin());
}

// Elaborates currently existing classes of equivalence by doing simulation.
// Simulation is saturated, when its iteration did not find any new classes.
void FraigChecker::saturation(Compiled &compiled,
                              EqClasses &classes,
                              GNet &premapped,
                              std::vector<BoolVector> simValues) {
  size_t index = 0;
  while (true) {
    EqClasses old = classes;
    if (simValues.empty()) {
      size_t temp = rand() % (1ULL << premapped.nSourceLinks());
      size_t simOut;
      compiled.simulate(simOut, temp);
      size_t classesSize = classes.size();
      for (size_t i = 0; i < classesSize; i++) {
        splitClass(classes, classes[0], compiled);
      }
    } else {
      BoolVector simOut(1);
      compiled.simulate(simOut, simValues[index]);
      splitClass(classes, classes[0], compiled);
      index += 1;
    }
    if (old == classes) {
      break;
    }
    if (index == simValues.size()) {
      break;
    }
  }    
}

CheckerResult FraigChecker::equivalent(GNet &lhs,
                                       GNet &rhs,
                                       Checker::GateIdMap &gmap) {
  if (!lhs.isComb() || !rhs.isComb()) {
    LOG_ERROR << "Checker works with combinational circuits only!" << std::endl;
    return CheckerResult::ERROR;
  }
  if (lhs.nSourceLinks() > simLimit || lhs.nTargetLinks() > simLimit) {
    LOG_ERROR << "Too many inputs / outputs for simulator!" << std::endl;
    return CheckerResult::ERROR;
  }
  // Creates the miter
  Checker::Hints hints = makeHints(lhs, rhs, gmap);
  GNet *mit = miter(lhs, rhs, hints);
  premapper::AigMapper mapper;
  GNet *premapped = (*mapper.map(*mit)).clone();
  premapped->sortTopologically();
  // Preparations for the simulation
  GateId outputId = model::Gate::INVALID;
  if (premapped->nTargetLinks() == 1) {
    outputId = (*premapped->targetLinks().begin()).source;
  }

  if (outputId == model::Gate::INVALID) {
    LOG_ERROR << "Incorrect number of OUT gates at miter: "
              <<  premapped->nTargetLinks() << std::endl;
    return CheckerResult::ERROR;
  }
  EqClasses classes = initClasses(premapped);
  // Is used to check that no gid pairs are going to be traversed more than once
  std::set<GidPair> checked;

  Checker checker;
  size_t time = 0;
  if (!classes.empty()) {
    mergeConsts(classes.front(), premapped, false);
    mergeConsts(classes.back(), premapped, true);
    classes.pop_back();
    std::swap(classes[0], classes.back());
    classes.pop_back();
  }
  classes = initClasses(premapped);
  while (!classes.empty() && time < tLimit) { 
    time++;
    std::vector<GateId> cutNodes;
    for (auto link : premapped->sourceLinks()) {
      cutNodes.push_back(link.target);
    }
    for (auto constant : premapped->constants()) {
        cutNodes.push_back(constant);
    }
    GateId first = 0;
    GateId second = 0;
    size_t classIndex = rand() % classes.size();

    //TODO: Figure out an optimised way to traverse pairs
    size_t maxPairs = classes[classIndex].size() *
                      (classes[classIndex].size() - 1);
    std::set<GidPair> attempted;
    while (first == second ||
          (checked.find({first, second}) != checked.end())) {
      first = classes[classIndex][rand() % classes[classIndex].size()];
      second = classes[classIndex][rand() % classes[classIndex].size()];
      if (first != second) {
        attempted.insert({first, second});
        attempted.insert({second, first});
      }

      if (first == second || (checked.find({first, second}) != checked.end())) {
        if (attempted.size() == maxPairs) {
          classes.erase(std::remove(classes.begin(),
                        classes.end(), classes[classIndex]));
          if (classes.empty()) {
            time += tLimit;
            break;
          }
          classIndex = rand() % classes.size();
          maxPairs = classes[classIndex].size() *
                     (classes[classIndex].size() - 1);
          attempted.clear();
        }
      }
    }
    if (!premapped->contains(first) || !premapped->contains(second)) {
      continue;    
    }
    bool position = true;
    for (auto *gate: premapped->gates()) {
      if (gate->id() == first) {
        position = false;
        break;
      }
      if (gate->id() == second) {
        position = true;
        break;
      }
    }
    if (!position) {
      std::swap(first, second);
    }
    BoundGNet bind1 = optimizer::extractCone(premapped, first, cutNodes);
    BoundGNet bind2 = optimizer::extractCone(premapped, second, cutNodes);
    GNet cone1 = *(bind1.net);
    GNet cone2 = *(bind2.net);
    cone1.sortTopologically();
    cone2.sortTopologically();

    checked.insert({second, first});
    checked.insert({first, second});
    if (cone1.nSourceLinks() != cone2.nSourceLinks()) {
      continue;
    }

    std::vector<GateId> inputs1;
    inputs1.reserve(cone1.nSourceLinks());
    std::vector<GateId> inputs2;
    inputs2.reserve(cone1.nSourceLinks());
    GateId out1 = (*cone1.targetLinks().begin()).source;
    GateId out2 = (*cone2.targetLinks().begin()).source;
    for (auto link : cone1.sourceLinks()) {
      inputs1.push_back(link.target);
    }
    for (auto link : cone2.sourceLinks()) {
      inputs2.push_back(link.target);
    }

    std::unordered_map<GateId, GateId> inOutMap;
    for (size_t i = 0; i < inputs1.size(); i++) {
      inOutMap[inputs1[i]] = inputs2[i];
    }
    inOutMap[out1] = out2;
    Checker::Hints coneHints = makeHints(cone1, cone2, inOutMap);
    GNet *coneMiter = miter(cone1, cone2, coneHints);

    std::vector<size_t> indexes = {};
    CheckerResult coneResult = checker.isEqualCombSatMiter(*coneMiter);
    bool sameInputsFlag = 1;
    for (size_t i = 0; i < bind1.inputBindings.size(); i++) {
      if ((bind1.inputBindings[i] == Gate::INVALID) &&
          (bind2.inputBindings[i] != Gate::INVALID)) {
        sameInputsFlag = 0;
      }
    }

    if (sameInputsFlag != 1) {
      continue;
    }
    if (coneResult.equal()) {
      premapped->mergeGates(first, second);
      // If a gate is merged, it should be deleted.
      classes[classIndex].erase(std::remove(classes[classIndex].begin(),
                                classes[classIndex].end(),
                                first));
      if (classes[classIndex].size() < 2) {
        classes.erase(std::remove(classes.begin(),
                                  classes.end(),
                                  classes[classIndex]));
      }
    } else {
      // If a gate is not merged, only the current pair should be deleted.
      BoolVector coneProof = coneResult.getCounterExample();
      BoolVector proof;
      size_t valIndex = 0;
      for (size_t i = 0; i < premapped->nSourceLinks(); i++) {
        if (bind1.inputBindings[i] == Gate::INVALID) {
          proof.push_back(false);
        } else {
          proof.push_back(coneProof[valIndex]);
          valIndex += 1;
        }
      }
      std::vector<BoolVector> newSimValues;
      for (size_t i = 0; i < proof.size(); i++) {
        BoolVector temp = proof;
        temp[i] = !proof[i];
        newSimValues.push_back(temp);
      }
      Compiled recompiled = makeCompiled(*premapped);
      // Elaboration of classes until saturation (specific values)
      saturation(recompiled, classes, *premapped, newSimValues);
    }
  }
  // TODO Add AIG rewriting.
  return checker.isEqualCombSatMiter(*premapped);
}

} // namespace eda::gate::debugger

