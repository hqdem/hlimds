//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/base_checker.h"
#include "gate/debugger/sat_checker.h"
#include "gate/model/gnet.h"
#include "gate/premapper/aigmapper.h"

#include <algorithm>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "cuddObj.hh"

namespace eda::gate::debugger {
using Gate = eda::gate::model::Gate;
using GNet = eda::gate::model::GNet;
using Hints = eda::gate::debugger::SatChecker::Hints;

/**
 * \brief Implements a logic equivalence checker (LEC) using Kuehlmann's method.
 *
 * The checking method is based on the article "Equivalence Checking Using Cuts
 * And Heaps" by A. Kuehlmann and F. Krohm (1997).
 */
class KChecker : public BaseChecker {
public:
  struct Vertex;

  struct BDDClass {
    BDD bddValue;
    std::shared_ptr<Vertex> v;
    int level = 0;
    bool containCutVar = false;
    std::vector<std::shared_ptr<Vertex>> cutPoints;

    BDDClass() {}
    BDDClass(BDD newBdd) : bddValue(newBdd) {}
    ~BDDClass() = default;
    void storeVertexAtBdd(std::shared_ptr<Vertex> vertex);
    void storeLevelAtBdd(int newLevel);
    int getLevelFromBdd();
    BDDClass getCutvarFromBdd();

    bool operator <(const BDDClass &other) const {
      return (bddValue.nodeCount() < other.bddValue.nodeCount() ||
              v->id < other.v->id);
    }
  };

  struct Vertex {
    int id;
    static int countId;
    static int primaryCountId;
    int level = 0;
    std::unordered_map<int, BDDClass> bdd;  // BDD on different levels
    bool outputSign = true;                 // 1 - positive, 0 - negative
    bool isPrimaryOutput = false;
    bool isPrimaryInput = false;
    bool containBDD = false;
    bool beenMerged = false;

    Vertex() {}
    Vertex(int id) : id(id) {}
    ~Vertex() = default;
    void storeBddAtVertex(std::set<BDDClass>::iterator newBdd,
                          int newLevel = 0);

    bool operator <(const Vertex &other) const { return (id < other.id); }
  };

  struct HashTableKey {
    std::shared_ptr<Vertex> v1;
    std::shared_ptr<Vertex> v2;
    bool sign1;
    bool sign2;

    HashTableKey(std::shared_ptr<Vertex> vL, std::shared_ptr<Vertex> vR,
                 bool signL, bool signR)
        : v1(vL), v2(vR), sign1(signL), sign2(signR) {}
    ~HashTableKey() = default;

    bool operator ==(const HashTableKey &other) const;
    bool operator !=(const HashTableKey &other) const {
      return !operator ==(other);
    }
  };

  struct HashForHashTableKey {
    std::size_t operator ()(const HashTableKey &k) const {
      return std::hash<int>()(k.v1->id) ^ std::hash<int>()(k.v2->id) ^
             std::hash<bool>()(k.sign1) ^ std::hash<bool>()(k.sign2);
    }
  };

  using VectorOfVertexPairs =
      std::vector<std::pair<std::shared_ptr<Vertex>, std::shared_ptr<Vertex>>>;
  using VertexMap = std::unordered_map<HashTableKey, std::shared_ptr<Vertex>,
                                       HashForHashTableKey>;

  const VectorOfVertexPairs &getMergedVertices() const {
    return mergedVertices;
  }
  const VertexMap &getHashTable() const { return hashTable; }

  /* Functions for testing */
  bool hashTableContains(std::vector<int> key, int value);
  bool valueIsPrimaryOutput(std::vector<int> key);
  bool primaryOutputSign(std::vector<int> key);
  void printHashTable();

  CheckerResult equivalent(GNet &lhs, GNet &rhs, const Hints &hints);
  CheckerResult equivalent(const GNet &lhs, const GNet &rhs,
                           const GateIdMap &gmap) const override;

private:
  int maxBddSize = 50;
  const int maxPossibleBddSize = 200;
  float step = 1.4;
  std::unordered_map<int, int> inputsBinding;
  std::unordered_map<int, int> outputsBinding;
  VectorOfVertexPairs mergedVertices;
  VertexMap hashTable;
  std::unordered_set<int> result;
  std::set<std::shared_ptr<Vertex>> primaryInputs;
  std::set<BDDClass> heap;
  Cudd mgr;

  std::shared_ptr<Vertex> getVertexById(int id, bool createNew = false);
  std::shared_ptr<Vertex> getPrimaryOutput(int id);
  int countV(int id);
  bool verticesAreMerged(std::shared_ptr<Vertex> v1,
                         std::shared_ptr<Vertex> v2);
  void equalKeys(HashTableKey key, std::shared_ptr<Vertex> v);
  void fillBindings(const Hints &hints);
  void bindingsAfterPremap(GateIdMap &gmap);
  bool gateHasSeveralOutputs(std::unordered_map<int, int> &GateIdToVertexId,
                             uint32_t gateId);
  bool isNegativeTarget(uint32_t target);
  bool leftIsBigger(uint32_t left, uint32_t right, uint32_t lOrR);
  void eliminateHangingVertices();
  void structHashing(GNet &net1, GNet &net2);
  void structHashingSingle(std::shared_ptr<GNet> premapped);

  std::pair<HashTableKey, std::shared_ptr<Vertex>> findValueInHashTable(int id);
  void putOnHeap(BDDClass &bddRes);
  void mergeVertices(std::shared_ptr<Vertex> vRes,
                     std::shared_ptr<Vertex> vOut);
  std::vector<HashTableKey> fanouts(std::shared_ptr<Vertex> v);
  bool vProcessedOrMerged(std::shared_ptr<Vertex> v);
  std::shared_ptr<Vertex> getVertexFromBdd(std::set<BDDClass>::iterator bddRes);
  std::shared_ptr<Vertex> getVertexFromBdd(BDD &bddRes);
  BDD getBddFromVertex(std::shared_ptr<Vertex> vOut, bool left, int level = 0);
  BDDClass getBddFromVertex(std::shared_ptr<Vertex> vOut, int level = 0);
  bool equalOutputs(std::shared_ptr<Vertex> v1, std::shared_ptr<Vertex> v2);
  bool notEqualOutputs(std::shared_ptr<Vertex> v1, std::shared_ptr<Vertex> v2);
  int cLevel(std::shared_ptr<Vertex> v);
  void cleanMergedVertices();
  CheckerResult getResult();
  void checkEquivalence(bool withCuts = false);
  void checkEquivalenceSingle(std::shared_ptr<Vertex> v1,
                              std::shared_ptr<Vertex> v2, bool withCuts);
  void eliminateFalseNegatives();
  void eliminateFalseNegativesSingle(std::shared_ptr<Vertex> v1,
                                     std::shared_ptr<Vertex> v2);
};
}  // namespace eda::gate::debugger
