//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/debugger/base_checker.h"
#include "gate/debugger/checker.h"
#include "gate/model/gnet.h"
#include "gate/premapper/aigmapper.h"

#include <algorithm>
#include <cudd.h>
#include <queue>
#include <unordered_map>
#include <vector>

namespace eda::gate::debugger {
using Gate = eda::gate::model::Gate;
using GNet = eda::gate::model::GNet;
using Hints = eda::gate::debugger::Checker::Hints;

/**
 * \brief Implements a logic equivalence checker (LEC) using Kuehlmann's method.
 *
 * The checking method is based on the article "Equivalence Checking Using Cuts
 * And Heaps" by A. Kuehlmann and F. Krohm (1997).
 */
class KChecker : public BaseChecker {
public:
  class Vertex;

  class BDD {
   public:
    int id;
    DdNode *bddValue;
    Vertex *v;
    int level;
    int size;

    void storeVertexAtBdd(Vertex v);
    Vertex getVertexFromBdd();
    void storeLevelAtBdd(int level);
    int getLevelFromBdd();
    BDD getCutvarFromBdd();
  };

  class Vertex {
  public:
    int id;
    static int countId;
    static int primaryCountId;
    int level;
    std::unordered_map<int, BDD> bdd;  // BDD on different levels
    bool outputSign = true;            // 1 - positive, 0 - negative
    bool isPrimaryOutput = false;
    bool isPrimaryInput = false;
    bool beenMerged = false;

    Vertex() {}
    Vertex(int VId) : id(VId) {}
    ~Vertex() = default;
    void storeBddAtVertex(BDD bdd, int level);
    BDD getBddFromVertex(int level);
    int cLevel();
  };

  struct StructHashKey {
    Vertex v1;
    Vertex v2;
    bool sign1;
    bool sign2;

    StructHashKey(Vertex vL, Vertex vR, bool signL, bool signR)
        : v1(vL), v2(vR), sign1(signL), sign2(signR) {}
    ~StructHashKey() = default;

    bool operator==(const StructHashKey &other) const {
      return (v1.id == other.v1.id && v2.id == other.v2.id &&
              sign1 == other.sign1 && sign2 == other.sign2);
    }

    bool operator!=(const StructHashKey &other) const {
      return (v1.id != other.v1.id || v2.id != other.v2.id ||
              sign1 != other.sign1 || sign2 != other.sign2);
    }
  };

  struct HashForStructHashKey {
    std::size_t operator()(const StructHashKey &k) const {
      return std::hash<int>()(k.v1.id) ^ std::hash<int>()(k.v2.id) ^
             std::hash<bool>()(k.sign1) ^ std::hash<bool>()(k.sign2);
    }
  };

  const std::vector<Vertex> &getMergedVertices() const {
    return mergedVertices;
  }

  const std::unordered_map<StructHashKey, Vertex, HashForStructHashKey> &
  getHashTable() const {
    return hashTable;
  }

  CheckerResult equivalent(GNet &lhs, GNet &rhs, const Hints &hints);
  CheckerResult equivalent(GNet &lhs, GNet &rhs, GateIdMap &gmap) override;

private:
  static const int maxBddSize = 50;
  static int maxPossibleBddSize;
  std::unordered_map<int, int> inputsBinding;
  std::unordered_map<int, int> outputsBinding;
  std::vector<Vertex> mergedVertices;
  std::unordered_map<StructHashKey, Vertex, HashForStructHashKey> hashTable;

  void equalKeys(StructHashKey key, Vertex v);
  void fillBindings(const Hints &hints);
  void bindingsAfterPremap(GateIdMap &gmap);
  bool gateHasSeveralOutputs(std::unordered_map<int, int> &GateIdToVertexId,
                             uint32_t gateId);
  bool isNegativeTarget(uint32_t target);
  bool leftIsBigger(uint32_t left, uint32_t right, uint32_t lOrR);
  void structHashing(GNet &net1, GNet &net2);
  void structHashingSingle(std::shared_ptr<GNet> premapped);
  void putOnHeap(std::priority_queue<BDD> heap, BDD bddRes);
  void mergeVertices(Vertex vRes, Vertex vOut);
};
}  // namespace eda::gate::debugger
