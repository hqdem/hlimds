//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/resynthesis/akers.h"

namespace eda::gate::optimizer::resynthesis {
//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

using Arguments = AkersAlgorithm::Arguments;
using ArgumentsSet = AkersAlgorithm::ArgumentsSet;
using Gate = eda::gate::model::Gate;
using GNet = eda::gate::model::GNet;

//===----------------------------------------------------------------------===//
// Constructors/Destructors
//===----------------------------------------------------------------------===//

AkersAlgorithm::AkersAlgorithm(const TruthTable &func, const TruthTable &care):
                               table(func, care), nVariables(func.num_vars()),
                               net(std::make_shared<GNet>()) {}

//===------------------------------------------------------------------------===//
// Convenience Methods
//===------------------------------------------------------------------------===//

std::shared_ptr<GNet> AkersAlgorithm::run(SignalList &inputs,
                                          Gate::Id &outputId) {

  for (uint32_t i = 0; i < nVariables; i++) {
    const Gate::Id inputId = net->addIn();
    const Gate::Signal input = Gate::Signal::always(inputId);
    signals.push_back(input);
    inputs.push_back(input);
  }

  while ((table.nColumns() != 3) && (table.nColumns() != 1)) {
    ColumnsToRemove columnsToRemove;

    Arguments gate = findBestGate(columnsToRemove);
    addMajGate(gate);

    if (!columnsToRemove.empty()) {
      table.eraseCol(columnsToRemove);
      columnsToRemove.clear();
    }

    if (!nCallElimFunc) {
      table.reduce();
    }
  }
  if (table.nColumns() == 3) {
    Arguments gate = {0, 1, 2};
    addMajGate(gate);
  } else {
    unsigned id = table.idColumn(0);
    Gate::Id inputId = 0;
    switch (id) {
      case 62:
        inputId = net->addZero();
        break;
      case 63:
        inputId = net->addOne();
        break;
      default:
        if ((id < 62) && (id > 30)) {
          inputId = net->addNot(signals[id - 31]);
        }
    }
    if (inputId) {
      signals.push_back(Gate::Signal::always(inputId));
    }
  }
  nMaj =  table.nMajGates;
  outputId = net->addOut(signals.back());
  net->sortTopologically();
  return net;
}
 
//===----------------------------------------------------------------------===//
// Internal Methods
//===----------------------------------------------------------------------===//

void AkersAlgorithm::addMajGate(const Arguments &gate) {
  assert(gate.size() == 3 && "Invalid number of inputs for a MAJ gate!");

  Gate::SignalList majInputs;
  
  for (const auto &i : gate) {
    unsigned id = table.idColumn(i);
    Gate::Id inputId;
    switch (id) {
      case 62:
        inputId = net->addZero();
        majInputs.push_back(Gate::Signal::always(inputId));
        break;
      case 63:
        inputId = net->addOne();
        majInputs.push_back(Gate::Signal::always(inputId));
        break;
      default:
        switch (id < 31 ? 1 : (id < 62 ? 2 : 3)) {
          case 1:
            majInputs.push_back(signals[id]);
            break;
          case 2:
            inputId = net->addNot(signals[id - 31]);
            majInputs.push_back(Gate::Signal::always(inputId));
            break;
          case 3:
            majInputs.push_back(signals[id - 64 + nVariables]);
            break;
        }
    }
  }

  const Gate::Id inputId = net->addMaj(majInputs[0],
                                       majInputs[1],
                                       majInputs[2]);
  signals.push_back(Gate::Signal::always(inputId));

  table.addMajColumn(gate);
}

Arguments AkersAlgorithm::findBestGate(ColumnsToRemove &columnsToRemove) {
  columnsToRemove.clear();

  CanditateList gates;
  Arguments args;
  ColumnsToRemove forRemoval;

  size_t nRows = table.nRows();
  unsigned degree;

  // map of columns and their essential ones.
  std::unordered_map<unsigned, RowNums> essentialOnes;
  pairEssentialRows.clear();
  // Filling maps.
  for (uint32_t i = 0; i < nRows; ++i) {
    for (uint32_t j = i + 1; j < nRows; ++j) {
      if (table.isDegreeOfTwo(table.getRow(i) & table.getRow(j), degree)) {
        essentialOnes[degree].insert(i);
        essentialOnes[degree].insert(j);
        pairEssentialRows[degree].push_back(std::make_pair(i, j));
      }
    }
  }
  // Selection of candidates (possible gates for the table).
  size_t nCols = nCallElimFunc ? nInnerColumns : table.nColumns();
  for (uint32_t i = 0; i < nCols; ++i) {
    for (const auto &gate : findGatesForColumnRemoval(essentialOnes[i], i)) {
      gates[gate].push_back(i);
    }
  }

  if (gates.empty()) {
    return findEliminatingOnesGate();
  }
  // Try to select more suitable set of arguments.
  auto it = gates.begin();
  args = it->first;
  forRemoval = it->second;

  ++it;
  for (; it != gates.end(); ++it) {
    if (it->second.size() > forRemoval.size()) {
      args = it->first;
      forRemoval = it->second;
    }
  }

  if (nCallElimFunc) {
    switch (forRemoval.size()) {
    case 1:
      return chooseGate(args, forRemoval, gates, columnsToRemove);
    
    case 2:
      return findEliminatingNColsGate(gates, columnsToRemove, 2);
    
    case 3:
      return findEliminatingNColsGate(gates, columnsToRemove, 3);
    }
  }

  return chooseGate(args, forRemoval, gates, columnsToRemove);
}

Arguments AkersAlgorithm::chooseGate(Arguments &candidate,
                                     ColumnsToRemove &forRemoval,
                                     const CanditateList &gates,
                                     ColumnsToRemove &columnsToRemove) {

  if ((forRemoval.size() != 1) || (mayDeleteRows(candidate, forRemoval))) {
    return setWhatFound(candidate, forRemoval, columnsToRemove);
  }
  auto it = gates.begin();
  ++it;
  for (; it != gates.end(); it++) {
    if (mayDeleteRows(it->first, it->second)) {
      return setWhatFound(it->first, it->second, columnsToRemove);
    }
  }
  return findEliminatingOnesGate();
}

Arguments AkersAlgorithm::findEliminatingNColsGate(CanditateList &gates,
  ColumnsToRemove &columnsToRemove, const unsigned n) {

  assert(((n == 2) || (n == 3)) && "Error of input variable n!");

  Arguments args;
  ColumnsToRemove forRemoval;
  
  unsigned i = n;
  do {
    --i;
    for (const auto &gate : gates) {
      if (gate.second.size() < n) {
        continue;
      }
      if (gate.second[i] < nInnerColumns) {
        args = gate.first;
        forRemoval = gate.second;
        if (mayDeleteRows(args, forRemoval)) {
          return setWhatFound(args, forRemoval, columnsToRemove);
        }
      }
    }
    bool firstTime = false;
    if (n == 3) {
      firstTime = (nCallElimFunc == 1);
    }
    if (!args.empty() && ((i == 2) || firstTime)) {
      return setWhatFound(args, forRemoval, columnsToRemove);
    }
  } while (i);

  CanditateList otherGates;
  for (const auto &gate : gates) {
    if (gate.second.size() < n) {
      otherGates[gate.first] = gate.second;
    }
  }

  if (otherGates.empty()) {
    return findEliminatingOnesGate();
  }

  if (n == 3) {
    return findEliminatingNColsGate(otherGates, columnsToRemove, 2);
  }

  args = otherGates.begin()->first;
  forRemoval = otherGates.begin()->second;
  return chooseGate(args, forRemoval, otherGates, columnsToRemove);
}

Arguments AkersAlgorithm::setWhatFound(const Arguments &args,
                                       const ColumnsToRemove &forRemoval,
                                       ColumnsToRemove &columnsToRemove) {

  nCallElimFunc = 0;
  columnsToRemove = forRemoval;
  return args;
}

ArgumentsSet AkersAlgorithm::findGatesForColumnRemoval
  (const RowNums &essentialRows, unsigned index) {

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

uint64_t AkersAlgorithm::countRemovedOnes(unsigned c1,
                                          unsigned c2,
                                          unsigned c3) {

  uint64_t counter = 0;
  std::vector<unsigned> args = {c1, c2, c3};

  for (unsigned i = 0; i < 3; ++i) {
    unsigned essArg = args[i];
    unsigned arg1 = args[(i + 1) % 3];
    unsigned arg2 = args[(i + 2) % 3];
    RowNums deletedOnes;
    RowNums cannotDelete;
    for (const auto &p : pairEssentialRows[essArg]) {
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

Arguments AkersAlgorithm::findEliminatingOnesGate() {
  if (!nCallElimFunc) {
    nInnerColumns = table.nColumns();
  }
  nCallElimFunc++;

  uint64_t counter = 0;
  uint64_t interCounter = 0;
  Arguments args;
  uint64_t columnsSize = table.nColumns();

  for (unsigned i = 0; i < nInnerColumns; i++) {
    for (unsigned j = i + 1; j < columnsSize; j++) {
      if (table.areInverse(i, j)) {
        continue;
      }
      for (unsigned k = j + 1; k < columnsSize; k++) {
        if ((table.areInverse(i, k)) || (table.areInverse(j, k))) {
          continue;
        }
        interCounter = countRemovedOnes(i, j, k);
        if (interCounter > counter) {
          counter = interCounter;
          args = {i, j, k};
        }
      }
    }
  }

  if (args.empty()) {
    args = {nCallElimFunc - 1, nCallElimFunc, nCallElimFunc + 1};
  }
  return args;
}

bool AkersAlgorithm::mayDeleteRows(const Arguments &args,
                                   const ColumnsToRemove &colsToErase) {
  table.addMajColumn(args);

  uint64_t mask = -1;
  for (const auto &col: colsToErase) {
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

}; // namespace eda::gate::optimizer::resynthesis
