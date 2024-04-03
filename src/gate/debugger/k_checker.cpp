//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/k_checker.h"

namespace eda::gate::debugger {
using HashTableKey = eda::gate::debugger::KChecker::HashTableKey;
using Vertex = eda::gate::debugger::KChecker::Vertex;
using BDDClass = eda::gate::debugger::KChecker::BDDClass;
using AigMapper = eda::gate::premapper::AigMapper;
using PreBasis = eda::gate::premapper::PreBasis;

int Vertex::primaryCountId = 0;
int Vertex::countId = 0;

void BDDClass::storeVertexAtBdd(std::shared_ptr<Vertex> vertex) { v = vertex; }

void BDDClass::storeLevelAtBdd(int newLevel) { level = newLevel; }

int BDDClass::getLevelFromBdd() { return level; }

void Vertex::storeBddAtVertex(std::set<BDDClass>::iterator newBdd,
                              int newLevel) {
  bdd.emplace(newLevel, *newBdd);
  containBDD = true;
  if (newLevel > level) {
    level = newLevel;
  }
}

bool HashTableKey::operator ==(const HashTableKey &other) const {
  return ((((v1 == nullptr && other.v1 == nullptr) ||
      (v1 != nullptr && other.v1 != nullptr && v1->id == other.v1->id)) &&
      ((v2 == nullptr && other.v2 == nullptr) ||
      (v2 != nullptr && other.v2 != nullptr && v2->id == other.v2->id)) &&
      sign1 == other.sign1 && sign2 == other.sign2));
}

bool KChecker::hashTableContains(std::vector<int> key, int value) {
  std::shared_ptr<Vertex> v1 = getVertexById(key[0]);
  std::shared_ptr<Vertex> v2 = getVertexById(key[1]);
  bool sign1 = key[2] == 1;
  bool sign2 = key[3] == 1;
  return hashTable.at({v1, v2, sign1, sign2}) == getVertexById(value);
}

bool KChecker::valueIsPrimaryOutput(std::vector<int> key) {
  std::shared_ptr<Vertex> v1 = getVertexById(key[0]);
  std::shared_ptr<Vertex> v2 = getVertexById(key[1]);
  bool sign1 = key[2] == 1;
  bool sign2 = key[3] == 1;
  return hashTable.at({v1, v2, sign1, sign2})->isPrimaryOutput;
}

bool KChecker::primaryOutputSign(std::vector<int> key) {
  std::shared_ptr<Vertex> v1 = getVertexById(key[0]);
  std::shared_ptr<Vertex> v2 = getVertexById(key[1]);
  bool sign1 = key[2] == 1;
  bool sign2 = key[3] == 1;
  return hashTable.at({v1, v2, sign1, sign2})->outputSign;
}

void KChecker::printHashTable() {
  for (const auto &pair : hashTable) {
    std::cout << "{" << pair.first.v1->id << ", " << pair.first.v2->id << ", "
              << pair.first.sign1 << ", " << pair.first.sign2
              << "} : " << pair.second->id << "\n";
  }
}

void KChecker::eliminateHangingVertices() {
  for (auto pair = hashTable.begin(); pair != hashTable.end();) {
    int id1 = pair->first.v1->id;
    int id2 = pair->first.v2->id;
    auto it1 =
        std::find_if(hashTable.begin(), hashTable.end(),
                     [&id1](const auto &el) { return el.second->id == id1; });
    auto it2 =
        std::find_if(hashTable.begin(), hashTable.end(),
                     [&id2](const auto &el) { return el.second->id == id2; });
    bool v1Hanging = !pair->first.v1->isPrimaryInput && it1 == hashTable.end();
    bool v2Hanging = !pair->first.v2->isPrimaryInput && it2 == hashTable.end();
    if (v1Hanging || v2Hanging) {
      int id = id2;
      if (v1Hanging) {
        id = id1;
      }
      auto itm =
          std::find_if(mergedVertices.begin(), mergedVertices.end(),
                       [id](const auto &el) { return el.second->id == id; });
      HashTableKey newKey = {pair->first.v1, itm->first, pair->first.sign1,
                             pair->first.sign2};
      if (v1Hanging) {
        newKey = {itm->first, pair->first.v2, pair->first.sign1,
                  pair->first.sign2};
      }
      hashTable.erase(pair->first);
      if (hashTable.find(newKey) != hashTable.end()) {
        equalKeys(newKey, pair->second);
      } else {
        hashTable.emplace(newKey, pair->second);
      }
    }
    if (v1Hanging || v2Hanging) {
      pair = hashTable.begin();
    } else {
      ++pair;
    }
  }
}

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
  eliminateHangingVertices();
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

      std::shared_ptr<Vertex> tempVertex = getVertexById(tempVertexId, true);
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
          tempVertex->isPrimaryOutput = true;

          auto outputGateId = gate.target;
          tempVertex->outputSign = true;
          if (isNegativeTarget(gate.target)) {
            tempVertex->outputSign = false;
            outputGateId = Gate::get(gate.target)->link(0).target;
          }

          outputsBinding[tempVertex->id] = outputsBinding[outputGateId];
          outputsBinding.erase(outputGateId);
          outputsBinding[outputsBinding.find(tempVertex->id)->second] =
              tempVertex->id;
          outputsBinding.erase(tempVertex->id);
        }
      }

      std::shared_ptr<Vertex> vLeft = getVertexById(leftVertexId, true);
      std::shared_ptr<Vertex> vRight = getVertexById(rightVertexId, true);

      if (isLeftPrimaryInput) {
        primaryInputs.emplace(vLeft);
      }
      if (isRightPrimaryInput) {
        primaryInputs.emplace(vRight);
      }
      vLeft->isPrimaryInput = isLeftPrimaryInput;
      vRight->isPrimaryInput = isRightPrimaryInput;
      HashTableKey key(vLeft, vRight, leftSign, rightSign);
      if (hashTable.find(key) != hashTable.end()) {
        equalKeys(key, tempVertex);
      } else {
        hashTable.emplace(key, tempVertex);
      }
    }
  }
}

std::shared_ptr<Vertex> KChecker::getVertexById(int id, bool createNew) {
  auto it =
      std::find_if(hashTable.begin(), hashTable.end(), [id](const auto &pair) {
        return (pair.first.v1 != nullptr && pair.first.v1->id == id) ||
               (pair.first.v2 != nullptr && pair.first.v2->id == id) ||
               (pair.second != nullptr && pair.second->id == id);
      });
  if (it == hashTable.end()) {
    if (createNew) {
      return std::make_shared<Vertex>(id);
    }
    return nullptr;
  }
  if (it->first.v1 != nullptr && it->first.v1->id == id) {
    return it->first.v1;
  }
  if (it->first.v2 != nullptr && it->first.v2->id == id) {
    return it->first.v2;
  }
  if (it->second != nullptr && it->second->id == id) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<Vertex> KChecker::getPrimaryOutput(int id) {
  if (countV(id) > 0) {
    return findValueInHashTable(id).second;
  }

  auto it = std::find_if(
      mergedVertices.begin(), mergedVertices.end(),
      [id](std::pair<std::shared_ptr<Vertex>, std::shared_ptr<Vertex>> pair) {
        return (pair.first != nullptr && pair.first->id == id) ||
               (pair.second != nullptr && pair.second->id == id);
      });
  if (it != mergedVertices.end()) {
    if (it->first->id == id) {
      return it->first;
    }
    return it->second;
  }
  return nullptr;
}

int KChecker::countV(int id) {
  int count = 0;
  for (const auto &pair : hashTable) {
    if (pair.first.v1->id == id || pair.first.v2->id == id ||
        pair.second->id == id) {
      count++;
    }
  }
  return count;
}

bool KChecker::verticesAreMerged(std::shared_ptr<Vertex> v1,
                                 std::shared_ptr<Vertex> v2) {
  auto it = std::find_if(
      mergedVertices.begin(), mergedVertices.end(),
      [v1,
       v2](std::pair<std::shared_ptr<Vertex>, std::shared_ptr<Vertex>> pair) {
        return (pair.first->id == v1->id && pair.second->id == v2->id) ||
               (pair.first->id == v2->id && pair.second->id == v1->id);
      });
  return it != mergedVertices.end();
}

void KChecker::equalKeys(HashTableKey key, std::shared_ptr<Vertex> v) {
  for (const auto &pair : hashTable) {
    HashTableKey tempKey = pair.first;
    std::shared_ptr<Vertex> tempVertex = pair.second;
    if (tempKey.v1->id == v->id || tempKey.v2->id == v->id) {
      HashTableKey newKey = {hashTable[key], tempKey.v2, tempKey.sign1,
                             tempKey.sign2};
      std::shared_ptr<Vertex> mergedVertex = tempKey.v1;
      if (tempKey.v2->id == v->id) {
        newKey = {tempKey.v1, hashTable[key], tempKey.sign1, tempKey.sign2};
        mergedVertex = tempKey.v2;
      }
      if (!verticesAreMerged(hashTable[key], mergedVertex)) {
        mergedVertices.push_back(std::make_pair(hashTable[key], mergedVertex));
        hashTable[key]->beenMerged = true;
        mergedVertex->beenMerged = true;
      }
      if (hashTable.find(newKey) != hashTable.end()) {
        hashTable.erase(tempKey);
        equalKeys(newKey, tempVertex);
        return;
      }

      hashTable.erase(tempKey);
      hashTable.emplace(newKey, tempVertex);
    } else if (tempKey == key && v->isPrimaryOutput) {
      mergedVertices.push_back(std::make_pair(hashTable[key], v));
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

std::pair<HashTableKey, std::shared_ptr<Vertex>> KChecker::findValueInHashTable(
    int id) {
  auto it =
      std::find_if(hashTable.begin(), hashTable.end(),
                   [&id](const auto &pair) { return pair.second->id == id; });
  return std::make_pair(it->first, it->second);
}

void KChecker::putOnHeap(BDDClass &bddRes) {
  if (bddRes.bddValue.nodeCount() < maxBddSize) {
    heap.emplace(bddRes);
  }
}

void KChecker::mergeVertices(std::shared_ptr<Vertex> vRes,
                             std::shared_ptr<Vertex> vOut) {
  for (auto it = heap.begin(); it != heap.end();) {
    if (it->v->id == vRes->id) {
      it = heap.erase(it);
    } else {
      ++it;
    }
  }
  equalKeys(findValueInHashTable(vOut->id).first, vRes);
}

std::vector<HashTableKey> KChecker::fanouts(std::shared_ptr<Vertex> v) {
  std::vector<HashTableKey> keyVector;
  for (const auto &pair : hashTable) {
    if (pair.first.v1->id == v->id || pair.first.v2->id == v->id) {
      keyVector.push_back(pair.first);
    }
  }
  return keyVector;
}

bool KChecker::vProcessedOrMerged(std::shared_ptr<Vertex> v) {
  bool processed =
      std::find_if(heap.begin(), heap.end(), [&v](const BDDClass &el) {
        return el.v && el.v->id == v->id;
      }) != heap.end();
  bool merged = !v->isPrimaryOutput && fanouts(v).empty();
  return processed || merged;
}

std::shared_ptr<Vertex> KChecker::getVertexFromBdd(
    std::set<BDDClass>::iterator bdd) {
  return bdd->v;
}

std::shared_ptr<Vertex> KChecker::getVertexFromBdd(BDD &bddRes) {
  auto it = std::find_if(
      heap.begin(), heap.end(),
      [bddRes](const BDDClass &el) { return el.bddValue == bddRes; });
  if (it != heap.end()) {
    return it->v;
  }
  return nullptr;
}

BDD KChecker::getBddFromVertex(std::shared_ptr<Vertex> vOut, bool left,
                               int level) {
  HashTableKey key = findValueInHashTable(vOut->id).first;
  BDD bdd;
  std::shared_ptr<Vertex> childV = nullptr;
  bool childSign;

  childV = left ? key.v1 : key.v2;
  childSign = left ? key.sign1 : key.sign2;
  if (childV->containBDD) {
    bdd = childV->bdd.at(level).bddValue;
  } else {
    auto it = std::find_if(
        heap.begin(), heap.end(),
        [childV](const BDDClass &el) { return el.v->id == childV->id; });
    if (it != heap.end()) {
      bdd = it->bddValue;
    } else {
      bdd = getBddFromVertex(childV, true) * getBddFromVertex(childV, false);
    }
  }
  if (!childSign) {
    bdd = ~bdd;
  }
  return bdd;
}

bool KChecker::equalOutputs(std::shared_ptr<Vertex> v1,
                            std::shared_ptr<Vertex> v2) {
  int maxLevel = std::min(v1->bdd.size(), v1->bdd.size());
  bool areEqual = false;
  if (v1->containBDD && v2->containBDD) {
    for (int level = 0; level < maxLevel; level++) {
      if (v1->bdd.at(level).bddValue == v2->bdd.at(level).bddValue) {
        areEqual = true;
        break;
      }
    }
  }

  return areEqual ||
         (v1->outputSign == v2->outputSign && verticesAreMerged(v1, v2));
}

bool KChecker::notEqualOutputs(std::shared_ptr<Vertex> v1,
                               std::shared_ptr<Vertex> v2) {
  int maxLevel = std::min(v1->bdd.size(), v1->bdd.size());
  bool areNotEqual = false;
  if (v1->containBDD && v2->containBDD) {
    for (int level = 0; level < maxLevel; level++) {
      if (v1->bdd.at(level).bddValue == ~v2->bdd.at(level).bddValue) {
        areNotEqual = true;
        break;
      }
    }
  }

  return areNotEqual ||
         (v1->outputSign != v2->outputSign && verticesAreMerged(v1, v2));
}

int KChecker::cLevel(std::shared_ptr<Vertex> v) {
  if (v->isPrimaryInput) {
    return 0;
  }

  std::shared_ptr<Vertex> vLeft = findValueInHashTable(v->id).first.v1;
  std::shared_ptr<Vertex> vRight = findValueInHashTable(v->id).first.v2;
  if (v->beenMerged) {
    return std::max(cLevel(vLeft), cLevel(vRight)) + 1;
  }
  return std::max(cLevel(vLeft), cLevel(vRight));
}

void KChecker::cleanMergedVertices() {
  for (auto it = mergedVertices.begin(); it != mergedVertices.end();) {
    bool noFanouts = fanouts(it->first).empty() && fanouts(it->second).empty();
    bool bothPrimaryOutputs =
         it->first->isPrimaryOutput && it->second->isPrimaryOutput;
    if (noFanouts || bothPrimaryOutputs) {
      it = mergedVertices.erase(it);
    } else {
      ++it;
    }
  }
}

CheckerResult KChecker::getResult() {
  if (result.size() == 2) {
    if (std::find(result.begin(), result.end(), CheckerResult::UNKNOWN) !=
        result.end()) {
      return CheckerResult::UNKNOWN;
    }
    return CheckerResult::NOTEQUAL;
  }
  if (result.size() == 1) {
    if (*result.begin() == CheckerResult::EQUAL) {
      return CheckerResult::EQUAL;
    }
    if (*result.begin() == CheckerResult::NOTEQUAL) {
      return CheckerResult::NOTEQUAL;
    }
    return CheckerResult::UNKNOWN;
  }
  return CheckerResult::UNKNOWN;
}

void KChecker::checkEquivalence(bool withCuts) {
  for (const auto &output : outputsBinding) {
    std::shared_ptr<Vertex> v1 = getPrimaryOutput(output.first);
    std::shared_ptr<Vertex> v2 = getPrimaryOutput(output.second);

    checkEquivalenceSingle(v1, v2, withCuts);
    heap.clear();
    if (getResult().isUnknown()) {
      break;
    }
  }
}

void KChecker::checkEquivalenceSingle(std::shared_ptr<Vertex> v1,
                                      std::shared_ptr<Vertex> v2,
                                      bool withCuts) {
  if (equalOutputs(v1, v2)) {
    result.emplace(CheckerResult::EQUAL);
    return;
  }
  if (notEqualOutputs(v1, v2)) {
    result.emplace(CheckerResult::NOTEQUAL);
    return;
  }

  if (withCuts) {
    for (auto it = mergedVertices.begin(); it != mergedVertices.end(); ++it) {
      std::shared_ptr<Vertex> c = nullptr;
      if (!fanouts(it->first).empty()) {
        c = it->first;
      } else {
        c = it->second;
      }

      BDDClass bdd(mgr.bddVar());
      int level = cLevel(c);
      bdd.storeLevelAtBdd(level);
      putOnHeap(bdd);
    }
  } else {
    for (std::shared_ptr<Vertex> i : primaryInputs) {
      BDDClass bdd(mgr.bddVar());
      bdd.storeVertexAtBdd(i);
      putOnHeap(bdd);
    }
  }

  while (!heap.empty()) {
    auto bdd = heap.begin();
    heap.erase(heap.begin());
    std::shared_ptr<Vertex> v = getVertexFromBdd(bdd);
    int level = withCuts ? bdd->level : 0;
    v->storeBddAtVertex(bdd, level);

    for (const HashTableKey &kOut : fanouts(v)) {
      std::shared_ptr<Vertex> vOut = hashTable[kOut];
      if (!vProcessedOrMerged(vOut)) {
        BDD bddLeft = getBddFromVertex(vOut, true, level);
        BDD bddRight = getBddFromVertex(vOut, false, level);
        BDDClass bddRes(bddLeft * bddRight);

        std::shared_ptr<Vertex> vRes = getVertexFromBdd(bddRes.bddValue);
        if (vRes != nullptr) {
          mergeVertices(vRes, vOut);
          if (equalOutputs(v1, v2)) {
            result.emplace(CheckerResult::EQUAL);
            return;
          }
          if (notEqualOutputs(v1, v2)) {
            result.emplace(CheckerResult::NOTEQUAL);
            return;
          }
        }
        bddRes.storeVertexAtBdd(vOut);
        bddRes.storeLevelAtBdd(level);

        putOnHeap(bddRes);
      }
    }
  }

  result.emplace(CheckerResult::UNKNOWN);
}

CheckerResult KChecker::equivalent(GNet &lhs, GNet &rhs, const Hints &hints) {
  if (!lhs.isComb() || !rhs.isComb()) {
    return CheckerResult::ERROR;
  }
  fillBindings(hints);
  structHashing(lhs, rhs);
  while (maxBddSize <= maxPossibleBddSize) {
    result.clear();
    checkEquivalence();
    if (!getResult().isUnknown()) {
      return getResult();
    }
    cleanMergedVertices();
    if (!mergedVertices.empty()) {
      result.clear();
      checkEquivalence(true);
      return getResult();
    }
    maxBddSize *= step;
  }
  return getResult();
}

CheckerResult KChecker::equivalent(const GNet &lhs, const GNet &rhs,
                                   const GateIdMap &gmap) const {
  return CheckerResult::EQUAL;
}

}  // namespace eda::gate::debugger