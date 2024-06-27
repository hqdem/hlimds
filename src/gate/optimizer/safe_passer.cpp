//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/safe_passer.h"

namespace eda::gate::optimizer {

/// SafePasser class

SafePasser &SafePasser::operator++() {
  if (saveNext != SubnetBuilder::invalidID) {
    entry = saveNext;
    saveNext = SubnetBuilder::invalidID;
    saveRoot = SubnetBuilder::invalidID;
  } else {
    EntryIterator::operator++();
  }
  callOnEachCell();
  while (isNewEntry.size() > entry && isNewEntry[entry] &&
         entry != SubnetBuilder::upperBoundID) {
    EntryIterator::operator++();
    callOnEachCell();
  }
  return *this;
}

SafePasser SafePasser::operator++(int) {
  SafePasser copyPasser(*this);
  ++(*this);
  return copyPasser;
}

SafePasser &SafePasser::operator--() {
  if (saveNext != SubnetBuilder::invalidID) {
    entry = saveNext;
    EntryIterator::operator--();
    saveNext = SubnetBuilder::invalidID;
    if (saveRoot != SubnetBuilder::invalidID) {
      if (entry == saveRoot) {
        EntryIterator::operator--();
      }
      saveRoot = SubnetBuilder::invalidID;
    }
  } else {
    EntryIterator::operator--();
  }
  callOnEachCell();
  while (isNewEntry.size() > entry && isNewEntry[entry] &&
         entry != SubnetBuilder::lowerBoundID) {
    EntryIterator::operator--();
    callOnEachCell();
  }
  return *this;
}

SafePasser SafePasser::operator--(int) {
  SafePasser copyPasser(*this);
  --(*this);
  return copyPasser;
}

void SafePasser::replace(
    const SubnetID rhsID,
    const SubnetBuilder::InOutMapping &rhsToLhsMapping,
    const std::function<float(const size_t)> *getCellWeight,
    const std::function<void(const size_t)> *onNewCell,
    const std::function<void(const size_t)> *onEqualDepth,
    const std::function<void(const size_t)> *onGreaterDepth) {

  const auto &rhsEntries = Subnet::get(rhsID).getEntries();
  prepareForReplace(rhsEntries.size() - 1, rhsToLhsMapping);
  auto &_isNewEntry = this->isNewEntry;
  std::function addNewCell = [&_isNewEntry, &onNewCell](const size_t entryID) {
    if (_isNewEntry.size() <= entryID) {
      _isNewEntry.resize(entryID + 1, false);
    }
    _isNewEntry[entryID] = true;

    // Callback passed by user
    if (onNewCell) {
      (*onNewCell)(entryID);
    }
  };

  builderToTransform->replace(rhsID, rhsToLhsMapping, getCellWeight,
                              &addNewCell, onEqualDepth, onGreaterDepth);
}

void SafePasser::replace(
    const SubnetBuilder &rhsBuilder,
    const SubnetBuilder::InOutMapping &rhsToLhsMapping,
    const std::function<void(const size_t)> *onNewCell,
    const std::function<void(const size_t)> *onEqualDepth,
    const std::function<void(const size_t)> *onGreaterDepth) {

  prepareForReplace(*(--rhsBuilder.end()), rhsToLhsMapping);
  auto &_isNewEntry = this->isNewEntry;
  std::function addNewCell = [&_isNewEntry, &onNewCell](const size_t entryID) {
    if (_isNewEntry.size() <= entryID) {
      _isNewEntry.resize(entryID + 1, false);
    }
    _isNewEntry[entryID] = true;

    // Callback passed by user
    if (onNewCell) {
      (*onNewCell)(entryID);
    }
  };
  builderToTransform->replace(rhsBuilder, rhsToLhsMapping,
                              &addNewCell, onEqualDepth, onGreaterDepth);
}

void SafePasser::finalizePass() {
  isNewEntry.clear();
  saveRoot = SubnetBuilder::invalidID;
  saveNext = SubnetBuilder::invalidID;
}

/// ReverseSafePasser

ReverseSafePasser &ReverseSafePasser::operator++() {
  SafePasser::operator--();
  return *this;
}

ReverseSafePasser ReverseSafePasser::operator++(int) {
  ReverseSafePasser copyPasser(*this);
  ++(*this);
  return copyPasser;
}

ReverseSafePasser &ReverseSafePasser::operator--() {
  SafePasser::operator++();
  return *this;
}

ReverseSafePasser ReverseSafePasser::operator--(int) {
  ReverseSafePasser copyPasser(*this);
  --(*this);
  return copyPasser;
}

} // namespace eda::gate::optimizer
