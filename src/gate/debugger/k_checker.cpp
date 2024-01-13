//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/k_checker.h"

namespace eda::gate::debugger {
using StructHashKey = eda::gate::debugger::KChecker::StructHashKey;
using Vertex = eda::gate::debugger::KChecker::Vertex;
using AigMapper = eda::gate::premapper::AigMapper;
using PreBasis = eda::gate::premapper::PreBasis;

int Vertex::primaryCountId = 0;
int Vertex::countId = 0;

void KChecker::structHashing(GNet &net1, GNet &net2) {
  GateIdMap gmap1;
  GateIdMap gmap2;
  std::shared_ptr<GNet> premapped1 =
      eda::gate::premapper::getPreMapper(PreBasis::AIG).map(net1, gmap1);
  std::shared_ptr<GNet> premapped2 =
      eda::gate::premapper::getPreMapper(PreBasis::AIG).map(net2, gmap2);

  bindingsAfterPremap(gmap1);
  bindingsAfterPremap(gmap2);

  int nGates1 = premapped1->nGates() - 1 - premapped1->nTargetLinks() -
                premapped1->nNegations();
  Vertex::countId = nGates1;
  structHashingSingle(premapped1);
  Vertex::countId = nGates1 + premapped2->nGates() -
                    premapped2->nTargetLinks() - premapped2->nNegations() -
                    premapped2->nSourceLinks();
  structHashingSingle(premapped2);
}

void KChecker::structHashingSingle(std::shared_ptr<GNet> premapped) {
  int tempVertexId = Vertex::countId;
  Vertex::countId--;
  std::unordered_map<int, int> GateIdToVertexId;

  for (auto tempGateId = premapped->gate(premapped->nGates() - 1)->id();
       tempGateId >= premapped->gate(0)->id(); --tempGateId) {
    if (!Gate::get(tempGateId)->isNegation() &&
        !Gate::get(tempGateId)->isSource() &&
        !Gate::get(tempGateId)->isTarget()) {
      auto leftInputId = Gate::get(tempGateId)->input(0).node();
      auto rightInputId = Gate::get(tempGateId)->input(1).node();
      int leftVertexId;
      int rightVertexId;
      bool leftSign = true;
      bool rightSign = true;
      bool isLeftPrimaryInput = false;
      bool isRightPrimaryInput = false;

      Vertex tempVertex(tempVertexId);
      tempVertexId--;

      if (Gate::get(leftInputId)->isNegation()) {
        leftSign = false;
        leftInputId = Gate::get(leftInputId)->input(0).node();
      }
      if (Gate::get(rightInputId)->isNegation()) {
        rightSign = false;
        rightInputId = Gate::get(rightInputId)->input(0).node();
      }

      if (Gate::get(rightInputId)->isSource()) {
        isRightPrimaryInput = true;
        rightVertexId = inputsBinding[rightInputId];
      } else {
        bool severalOutputs =
            gateHasSeveralOutputs(GateIdToVertexId, rightInputId);

        if (GateIdToVertexId.find(rightInputId) != GateIdToVertexId.end()) {
          rightVertexId = GateIdToVertexId[rightInputId];
        } else {
          if (leftIsBigger(leftInputId, rightInputId, leftInputId)) {
            rightVertexId = Vertex::countId - 1;
          } else {
            rightVertexId = Vertex::countId;
          }
          Vertex::countId--;
        }

        if (severalOutputs) {
          GateIdToVertexId.emplace(rightInputId, rightVertexId);
        }
      }

      if (Gate::get(leftInputId)->isSource()) {
        isLeftPrimaryInput = true;
        leftVertexId = inputsBinding[leftInputId];
      } else {
        bool severalOutputs =
            gateHasSeveralOutputs(GateIdToVertexId, leftInputId);

        if (GateIdToVertexId.find(leftInputId) != GateIdToVertexId.end()) {
          leftVertexId = GateIdToVertexId[leftInputId];
        } else {
          if (leftIsBigger(leftInputId, rightInputId, rightInputId)) {
            leftVertexId = Vertex::countId + 1;
          } else {
            leftVertexId = Vertex::countId;
          }
          Vertex::countId--;
        }

        if (severalOutputs) {
          GateIdToVertexId.emplace(leftInputId, leftVertexId);
        }
      }

      for (const auto &gate : Gate::get(tempGateId)->links()) {
        if (Gate::get(gate.target)->isTarget() ||
            isNegativeTarget(gate.target)) {
          tempVertex.isPrimaryOutput = true;

          auto outputGateId = gate.target;
          tempVertex.outputSign = true;
          if (isNegativeTarget(gate.target)) {
            tempVertex.outputSign = false;
            outputGateId = Gate::get(gate.target)->link(0).target;
          }

          outputsBinding[tempVertex.id] = outputsBinding[outputGateId];
          outputsBinding.erase(outputGateId);
          outputsBinding[outputsBinding.find(tempVertex.id)->second] =
              tempVertex.id;
          outputsBinding.erase(tempVertex.id);
        }
      }

      Vertex vLeft(leftVertexId), vRight(rightVertexId);
      vLeft.isPrimaryInput = isLeftPrimaryInput;
      vRight.isPrimaryInput = isRightPrimaryInput;
      StructHashKey key(vLeft, vRight, leftSign, rightSign);
      if (hashTable.find(key) != hashTable.end()) {
        equalKeys(key, tempVertex);
      } else {
        hashTable.emplace(key, tempVertex);
      }
    }
  }
}

void KChecker::equalKeys(StructHashKey key, Vertex v) {
  for (const auto &pair : hashTable) {
    StructHashKey tempKey = pair.first;
    Vertex tempVertex = pair.second;
    if (tempKey.v1.id == v.id || tempKey.v2.id == v.id) {
      StructHashKey newKey = {hashTable[key], tempKey.v2, tempKey.sign1,
                              tempKey.sign2};
      if (tempKey.v2.id == v.id) {
        newKey = {tempKey.v1, hashTable[key], tempKey.sign1, tempKey.sign2};
      }
      mergedVertices.push_back(hashTable[key]);
      auto it = hashTable.find(newKey);
      if (it != hashTable.end()) {
        hashTable.erase(tempKey);
        equalKeys(newKey, tempVertex);
        return;
      }

      hashTable.erase(tempKey);
      hashTable.emplace(newKey, tempVertex);
    } else if (tempKey == key && v.isPrimaryOutput) {
      mergedVertices.push_back(hashTable[key]);
      return;
    }
  }
}

void KChecker::fillBindings(const Hints &hints) {
  Vertex::primaryCountId = 0;
  Vertex::countId = 0;

  for (const auto &binding : *hints.sourceBinding) {
    inputsBinding[binding.first.target] = Vertex::primaryCountId;
    inputsBinding[binding.second.target] = Vertex::primaryCountId;
    Vertex::primaryCountId++;
  }

  for (const auto &binding : *hints.targetBinding) {
    outputsBinding[binding.first.target] = binding.second.target;
  }
}

void KChecker::bindingsAfterPremap(GateIdMap &gmap) {
  for (const auto &el : gmap) {
    if (Gate::get(el.first)->isSource()) {
      inputsBinding[el.second] = inputsBinding[el.first];
      inputsBinding.erase(el.first);
    } else if (Gate::get(el.first)->isTarget()) {
      outputsBinding[el.second] = outputsBinding[el.first];
      outputsBinding.erase(el.first);
      outputsBinding[outputsBinding.find(el.second)->second] = el.second;
      outputsBinding.erase(el.second);
    }
  }
}

bool KChecker::gateHasSeveralOutputs(
    std::unordered_map<int, int> &GateIdToVertexId, uint32_t gateId) {
  return (GateIdToVertexId.find(gateId) == GateIdToVertexId.end() &&
          (Gate::get(gateId)->fanout() > 1 ||
           (Gate::get(Gate::get(gateId)->link(0).target)->isNegation() &&
            Gate::get(Gate::get(gateId)->link(0).target)->fanout() > 1)));
}

bool KChecker::isNegativeTarget(uint32_t target) {
  return (Gate::get(target)->isNegation() &&
          Gate::get(Gate::get(target)->link(0).target)->isTarget());
}

bool KChecker::leftIsBigger(uint32_t left, uint32_t right, uint32_t lOrR) {
  return (left > right && !Gate::get(lOrR)->isSource());
}

CheckerResult KChecker::equivalent(GNet &lhs, GNet &rhs, const Hints &hints) {
  fillBindings(hints);
  structHashing(lhs, rhs);
  return CheckerResult::EQUAL;
}

CheckerResult KChecker::equivalent(GNet &lhs, GNet &rhs, GateIdMap &gmap) {
  return CheckerResult::EQUAL;
}

}  // namespace eda::gate::debugger