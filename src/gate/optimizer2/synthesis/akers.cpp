//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer2/synthesis/akers.h"

namespace eda::gate::optimizer2::synthesis {
//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

using Arguments    = AkersAlgorithm::Arguments;
using ArgumentsSet = AkersAlgorithm::ArgumentsSet;
using Link         = eda::gate::model::Subnet::Link;
using Subnet       = eda::gate::model::Subnet;
using SubnetID     = eda::gate::model::SubnetID;

//===----------------------------------------------------------------------===//
// Synthesize Methods
//===----------------------------------------------------------------------===//

SubnetID AkersAlgorithm::synthesize(const TruthTable &func, uint16_t maxArity) {
  /// TODO: Wrong argument processing is needed.
  assert(maxArity > 2 && "Arity of MAJ gate should be >= 3!");
  TruthTable care(func.num_vars());
  std::string bitsCare;
  bitsCare.assign(func.num_bits(), '1');
  kitty::create_from_binary_string(care, bitsCare);

  return run(func, care);
}

//===----------------------------------------------------------------------===//
// Internal Methods
//===----------------------------------------------------------------------===//

SubnetID AkersAlgorithm::run(const TruthTable &func, const TruthTable &care) {
  // Initialize the unitized table.
  UnitizedTable table;
  table.initialize(func, care);
  uint32_t nVariables = func.num_vars();
  // Create variables for building the Subnet.
  SubBuild subBuild;
  for (uint32_t i = 0; i < nVariables; i++) {
    size_t cellId = subBuild.builder.addInput().idx;
    subBuild.idx.push_back(cellId);
  }

  ElimOnesInfo onesInfo;
  onesInfo.nCall = 0;
  onesInfo.nInner = table.nColumns();
  ConstantId cid;

  while ((table.nColumns() != 3) && (table.nColumns() != 1)) {
    Candidate candidate = findBestGate(table, onesInfo);
    addMajGate(table, subBuild, candidate.args, nVariables, cid);

    if (!candidate.toRemove.empty()) {
      table.eraseCol(candidate.toRemove);
    }

    if (!onesInfo.nCall) {
      table.reduce();
    }
  }
  bool inv = false;
  if (table.nColumns() == 3) {
    Arguments gate = {0, 1, 2};
    addMajGate(table, subBuild, gate, nVariables, cid);
  } else {
    unsigned id = table.idColumn(0);
    size_t cellId = 0;
    bool flag = false;
    switch (id) {
      case 62:
        cellId = subBuild.builder.addCell(model::ZERO).idx;
        flag = true;
      break;
      case 63:
        cellId = subBuild.builder.addCell(model::ONE).idx;
        flag = true;
      break;
      default:
        if ((id < 62) && (id > 30)) {
          cellId = subBuild.idx[id - 31];
          flag = true;
          inv = true;
        }
    }
    if (flag) {
      subBuild.idx.push_back(cellId);
    } else {
      if (id < 31) {
        cellId = subBuild.idx[id];
        subBuild.idx.push_back(cellId);
      }
    }
  }
  const Link link(subBuild.idx.back(), inv);
  subBuild.builder.addOutput(link);
  return subBuild.builder.make();
}

void AkersAlgorithm::addMajGate(UnitizedTable &table, SubBuild &subBuild,
                                const Arguments &gate, uint32_t nVariables,
                                ConstantId &cid) {

  assert(gate.size() == 3 && "Invalid number of inputs for a MAJ gate!");

  std::vector<Link> links;

  for (const auto &i : gate) {
    unsigned id = table.idColumn(i);
    size_t cellId = 0;
    switch (id) {
      case 62:
        if (!cid.hasZero) {
          cid.zeroId = subBuild.builder.addCell(model::ZERO).idx;
          cid.hasZero = true;
        }
        cellId = cid.zeroId;
        links.push_back(Link(cellId));
      break;
      case 63:
        if (!cid.hasOne) {
          cid.oneId = subBuild.builder.addCell(model::ONE).idx;
          cid.hasOne = true;
        }
        cellId = cid.oneId;
        links.push_back(Link(cellId));
      break;
      default:
        switch (id < 31 ? 1 : (id < 62 ? 2 : 3)) {
          case 1:
            links.push_back(Link(subBuild.idx[id]));
          break;
          case 2:
            links.push_back(Link(subBuild.idx[id - 31], true));
          break;
          case 3:
            links.push_back(Link(subBuild.idx[id - 64 + nVariables]));
          break;
        }
    }
  }

  const size_t majId = subBuild.builder.addCell(model::MAJ, links[0],
                                                            links[1],
                                                            links[2]).idx;
  subBuild.idx.push_back(majId);

  table.addMajColumn(gate);
}

Candidate AkersAlgorithm::findBestGate(UnitizedTable &table,
                                       ElimOnesInfo &onesInfo) {

  CanditateList gates;
  Candidate candidate;

  size_t nRows = table.nRows();
  unsigned degree;

  // map of columns and their essential ones.
  std::unordered_map<unsigned, RowNums> essenOnes;
  EssentialEdge edges;
  // Filling maps.
  for (uint32_t i = 0; i < nRows; ++i) {
    for (uint32_t j = i + 1; j < nRows; ++j) {
      if (table.isDegreeOfTwo(table.getRow(i) & table.getRow(j), degree)) {
        essenOnes[degree].insert(i);
        essenOnes[degree].insert(j);
        edges[degree].push_back(std::make_pair(i, j));
      }
    }
  }
  // Selection of candidates (possible gates for the table).
  size_t nCols = onesInfo.nCall ? onesInfo.nInner : table.nColumns();
  for (uint32_t i = 0; i < nCols; ++i) {
    for (const auto &gate : findGatesForColumnRemoval(table, essenOnes[i], i)) {
      gates[gate].push_back(i);
    }
  }

  if (gates.empty()) {
    return findEliminatingOnesGate(table, edges, onesInfo);
  }
  // Try to select more suitable set of arguments.
  auto it = gates.begin();
  candidate.args = it->first;
  candidate.toRemove = it->second;

  ++it;
  for (; it != gates.end(); ++it) {
    if (it->second.size() > candidate.toRemove.size()) {
      candidate.args = it->first;
      candidate.toRemove = it->second;
    }
  }

  if (onesInfo.nCall) {
    switch (candidate.toRemove.size()) {
    case 1:
      return chooseGate(table, edges, candidate, gates, onesInfo);
    
    case 2:
      return findEliminatingNColsGate(table, edges, gates, onesInfo, 2);
    
    case 3:
      return findEliminatingNColsGate(table, edges, gates, onesInfo, 3);
    }
  }

  return chooseGate(table, edges, candidate, gates, onesInfo);
}

Candidate AkersAlgorithm::chooseGate(UnitizedTable &table,
                                     EssentialEdge &edges,
                                     Candidate &candidate,
                                     const CanditateList &gates,
                                     ElimOnesInfo &onesInfo) {

  if ((candidate.toRemove.size() != 1) || (mayDeleteRows(table, candidate))) {
    return setWhatFound(candidate, onesInfo);
  }
  auto it = gates.begin();
  ++it;
  for (; it != gates.end(); it++) {
    candidate.args = it->first;
    candidate.toRemove = it->second;
    if (mayDeleteRows(table, candidate)) {
      return setWhatFound(candidate, onesInfo);
    }
  }
  return findEliminatingOnesGate(table, edges, onesInfo);
}

Candidate AkersAlgorithm::findEliminatingNColsGate(UnitizedTable &table,
                                                   EssentialEdge &edges,
                                                   CanditateList &gates,
                                                   ElimOnesInfo &onesInfo,
                                                   const unsigned n) {

  assert(((n == 2) || (n == 3)) && "Error of input variable n!");

  Candidate candidate;

  unsigned i = n;
  do {
    --i;
    for (const auto &gate : gates) {
      if (gate.second.size() < n) {
        continue;
      }
      if (gate.second[i] < onesInfo.nInner) {
        candidate.args = gate.first;
        candidate.toRemove = gate.second;
        if (mayDeleteRows(table, candidate)) {
          return setWhatFound(candidate, onesInfo);
        }
      }
    }
    bool firstTime = false;
    if (n == 3) {
      firstTime = (onesInfo.nCall == 1);
    }
    if (!candidate.args.empty() && ((i == 2) || firstTime)) {
      return setWhatFound(candidate, onesInfo);
    }
  } while (i);

  CanditateList otherGates;
  for (const auto &gate : gates) {
    if (gate.second.size() < n) {
      otherGates[gate.first] = gate.second;
    }
  }

  if (otherGates.empty()) {
    return findEliminatingOnesGate(table, edges, onesInfo);
  }

  if (n == 3) {
    return findEliminatingNColsGate(table, edges, otherGates, onesInfo, 2);
  }

  candidate.args = otherGates.begin()->first;
  candidate.toRemove = otherGates.begin()->second;
  return chooseGate(table, edges, candidate, otherGates, onesInfo);
}

Candidate AkersAlgorithm::setWhatFound(const Candidate &candidate,
                                       ElimOnesInfo &onesInfo) {

  onesInfo.nCall = 0;
  return candidate;
}

ArgumentsSet AkersAlgorithm::findGatesForColumnRemoval
  (const UnitizedTable &table, const RowNums &essentialRows, unsigned index) {

  ArgumentsSet argsSet;

  uint64_t columnsSize = table.nColumns();

  for (unsigned i = 0; i < columnsSize; i++) {
    if ((index == i) || (table.areInverse(index, i))) {
      continue;
    }
    for (unsigned j = i + 1; j < columnsSize; j++) {
      if ((index == j) ||
          (table.areInverse(index, j)) ||
          (table.areInverse(i, j))) {
        continue;
      }
      bool wasFound = true;
      for (const auto &row : essentialRows) {
        if (!table.getBit(row, i) && !table.getBit(row, j)) {
          wasFound = false;
          break;
        }
      }
      if (wasFound) {
        Arguments gate;
        gate.insert(index);
        gate.insert(i);
        gate.insert(j);
        argsSet.insert(gate);
      }
    }
  }
  return argsSet;
}

uint64_t AkersAlgorithm::countRemoved(const UnitizedTable &table,
                                      EssentialEdge &edges,
                                      unsigned c1, unsigned c2, unsigned c3) {

  uint64_t counter = 0;
  std::vector<unsigned> args = {c1, c2, c3};

  for (unsigned i = 0; i < 3; ++i) {
    unsigned essArg = args[i];
    unsigned arg1 = args[(i + 1) % 3];
    unsigned arg2 = args[(i + 2) % 3];
    RowNums deletedOnes;
    RowNums cannotDelete;
    for (const auto &p : edges[essArg]) {
      bool bit1 = table.getBit(p.first, arg1) || table.getBit(p.first, arg2);
      bool bit2 = table.getBit(p.second, arg1) || table.getBit(p.second, arg2);
      if (bit1 && bit2) {
        if (cannotDelete.find(p.first) == cannotDelete.end()) {
          incCounter(counter, deletedOnes, p.first);
        }
        if (cannotDelete.find(p.second) == cannotDelete.end()) {
          incCounter(counter, deletedOnes, p.second);
        }
      } else {
        if (cannotDelete.find(p.first) == cannotDelete.end()) {
          decCounter(counter, cannotDelete, deletedOnes, p.first);
        }
        if (cannotDelete.find(p.second) == cannotDelete.end()) {
          decCounter(counter, cannotDelete, deletedOnes, p.second);
        }
      }
    }
  }
  return counter;
}

void AkersAlgorithm::incCounter(uint64_t &counter,
                                RowNums &toRemove,
                                uint32_t rowNum) {

  auto pair = toRemove.insert(rowNum);
  if (pair.second) {
    counter++;
  }
}

void AkersAlgorithm::decCounter(uint64_t &counter, RowNums &cantRemove,
                                RowNums &toRemove, uint32_t rowNum) {

  cantRemove.insert(rowNum);
  size_t flag = toRemove.erase(rowNum);
  if (flag) {
    counter--;
  }
}

Candidate AkersAlgorithm::findEliminatingOnesGate(const UnitizedTable &table,
                                                  EssentialEdge &edges,
                                                  ElimOnesInfo &onesInfo) {

  if (!onesInfo.nCall) {
    onesInfo.nInner = table.nColumns();
  }
  onesInfo.nCall++;

  uint64_t counter = 0;
  uint64_t interCounter = 0;
  Candidate candidate;
  uint64_t columnsSize = table.nColumns();

  for (unsigned i = 0; i < onesInfo.nInner; i++) {
    for (unsigned j = i + 1; j < columnsSize; j++) {
      if (table.areInverse(i, j)) {
        continue;
      }
      for (unsigned k = j + 1; k < columnsSize; k++) {
        if ((table.areInverse(i, k)) || (table.areInverse(j, k))) {
          continue;
        }
        interCounter = countRemoved(table, edges, i, j, k);
        if (interCounter > counter) {
          counter = interCounter;
          candidate.args = {i, j, k};
        }
      }
    }
  }

  if (candidate.args.empty()) {
    candidate.args = {onesInfo.nCall - 1, onesInfo.nCall, onesInfo.nCall + 1};
  }
  return candidate;
}

bool AkersAlgorithm::mayDeleteRows(UnitizedTable &table,
                                   const Candidate &candidate) {

  table.addMajColumn(candidate.args);

  uint64_t mask = -1;
  for (const auto &col: candidate.toRemove) {
    mask &= ~(1ull << col);
  }

  uint64_t columnsSize = table.nColumns();
  uint64_t tableSize = table.nRows();

  for (uint32_t i = 0; i < tableSize; i++) {
    for (uint32_t j = i + 1; j < tableSize; j++) {
      uint64_t r1 = table.getRow(i) & mask;
      uint64_t r2 = table.getRow(j) & mask;
      uint64_t res = r1 | r2;
      if ((res == r1) || (res == r2)) {
        table.eraseCol(columnsSize - 1);
        table.nMajGates--;
        return true;
      }
    }
  }
  table.eraseCol(columnsSize - 1);
  table.nMajGates--;
  return false;
}

} // namespace eda::gate::optimizer2::synthesis
