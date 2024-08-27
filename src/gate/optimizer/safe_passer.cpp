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

SafePasser::SafePasser(
  EntryIterator iter,
  const std::function<void(const uint32_t)> *onEachEntry) :
    EntryIterator(iter),
    builderToTransform(const_cast<SubnetBuilder *>(builder)),
    onEachEntry(onEachEntry) {

  if (onEachEntry) {
    (*onEachEntry)(entry);
  }
  if (entry != SubnetBuilder::lowerBoundID &&
      entry != SubnetBuilder::upperBoundID &&
      entry != SubnetBuilder::invalidID &&
      isPassedEntry.size() <= entry) {
    isPassedEntry.resize(entry + 1);
    isPassedEntry[entry] = true;
  }
};

void SafePasser::changeItParent(const Direction dir) {
  dir == FORWARD ? EntryIterator::operator++() : EntryIterator::operator--();
}

void SafePasser::changeIt(const Direction dir) {
  checkDirection(dir);
  if (saveNext != SubnetBuilder::invalidID) {
    entry = saveNext;
    saveNext = SubnetBuilder::invalidID;
  } else {
    changeItParent(dir);
  }
  callOnEachCell();
  while (((isNewEntry.size() > entry && isNewEntry[entry]) ||
          (isPassedEntry.size() > entry && isPassedEntry[entry]))) {
    if ((entry == SubnetBuilder::upperBoundID && dir == FORWARD) ||
        (entry == SubnetBuilder::lowerBoundID && dir == BACKWARD)) {
      break;
    }
    changeItParent(dir);
    callOnEachCell();
  }
  if ((entry == SubnetBuilder::upperBoundID && dir == FORWARD) ||
      (entry == SubnetBuilder::lowerBoundID && dir == BACKWARD)) {
    return;
  }
  if (isPassedEntry.size() <= entry) {
    isPassedEntry.resize(entry + 1);
  }
  isPassedEntry[entry] = true;
}

SafePasser &SafePasser::operator++() {
  changeIt(FORWARD);
  return *this;
}

SafePasser SafePasser::operator++(int) {
  SafePasser copyPasser(*this);
  ++(*this);
  return copyPasser;
}

SafePasser &SafePasser::operator--() {
  changeIt(BACKWARD);
  return *this;
}

SafePasser SafePasser::operator--(int) {
  SafePasser copyPasser(*this);
  --(*this);
  return copyPasser;
}

void SafePasser::replace(
    const SubnetObject &rhs,
    const InOutMapping &rhsToLhsMapping,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onEqualDepth,
    const CellActionCallback *onGreaterDepth,
    const CellCallbackCondition *onRecomputedDepth) {

  const auto oldRootDepth = builder->getDepth(entry);
  const bool rootLastDepth = builder->getLastWithDepth(oldRootDepth) == entry;
  if (rhs.hasBuilder()) {
    const auto &rhsBuilder = rhs.builder();
    prepareForReplace(*(--rhsBuilder.end()), rhsToLhsMapping);
  } else {
    const auto &rhsSubnet = rhs.object();
    prepareForReplace(rhsSubnet.getMaxIdx(), rhsToLhsMapping);
  }

  auto &_isNewEntry = this->isNewEntry;
  std::function addNewCell = [&_isNewEntry, &onNewCell](const uint32_t entryID) {
    if (_isNewEntry.size() <= entryID) {
      _isNewEntry.resize(entryID + 1, false);
    }
    _isNewEntry[entryID] = true;

    // Callback passed by user
    if (onNewCell) {
      (*onNewCell)(entryID);
    }
  };

  const auto &_builder = builder;
  std::function onRecompDepthWrap =
      [&onRecomputedDepth, oldRootDepth, &_builder](const uint32_t entryID) {
    if (onRecomputedDepth) {
      (*onRecomputedDepth)(entryID, oldRootDepth, _builder->getDepth(entryID));
    }
  };

  builderToTransform->replace(rhs, rhsToLhsMapping,
                              &addNewCell, onEqualDepth,
                              onGreaterDepth, &onRecompDepthWrap);
  recomputeNext(oldRootDepth, rootLastDepth);
}

void SafePasser::replace(
    const SubnetID rhsID,
    const InOutMapping &rhsToLhsMapping,
    const CellWeightProvider *getCellWeight,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onEqualDepth,
    const CellActionCallback *onGreaterDepth,
    const CellCallbackCondition *onRecomputedDepth) {

  const auto oldRootDepth = builder->getDepth(entry);
  const bool rootLastDepth = builder->getLastWithDepth(oldRootDepth) == entry;
  const auto &rhsEntries = Subnet::get(rhsID).getEntries();
  prepareForReplace(rhsEntries.size() - 1, rhsToLhsMapping);
  auto &_isNewEntry = this->isNewEntry;
  std::function addNewCell = [&_isNewEntry, &onNewCell](const uint32_t entryID) {
    if (_isNewEntry.size() <= entryID) {
      _isNewEntry.resize(entryID + 1, false);
    }
    _isNewEntry[entryID] = true;

    // Callback passed by user
    if (onNewCell) {
      (*onNewCell)(entryID);
    }
  };

  const auto &_builder = builder;
  std::function onRecompDepthWrap =
      [&onRecomputedDepth, oldRootDepth, &_builder](const uint32_t entryID) {
    if (onRecomputedDepth) {
      (*onRecomputedDepth)(entryID, oldRootDepth, _builder->getDepth(entryID));
    }
  };

  builderToTransform->replace(rhsID, rhsToLhsMapping, getCellWeight,
                              &addNewCell, onEqualDepth, onGreaterDepth,
                              &onRecompDepthWrap);
  recomputeNext(oldRootDepth, rootLastDepth);
}

void SafePasser::replace(
    const SubnetBuilder &rhsBuilder,
    const InOutMapping &rhsToLhsMapping,
    const CellActionCallback *onNewCell,
    const CellActionCallback *onEqualDepth,
    const CellActionCallback *onGreaterDepth,
    const CellCallbackCondition *onRecomputedDepth) {

  const auto oldRootDepth = builder->getDepth(entry);
  const bool rootLastDepth = builder->getLastWithDepth(oldRootDepth) == entry;
  prepareForReplace(*(--rhsBuilder.end()), rhsToLhsMapping);
  auto &_isNewEntry = this->isNewEntry;
  std::function addNewCell = [&_isNewEntry, &onNewCell](const uint32_t entryID) {
    if (_isNewEntry.size() <= entryID) {
      _isNewEntry.resize(entryID + 1, false);
    }
    _isNewEntry[entryID] = true;

    // Callback passed by user
    if (onNewCell) {
      (*onNewCell)(entryID);
    }
  };

  const auto &_builder = builder;
  std::function onRecompDepthWrap =
      [&onRecomputedDepth, oldRootDepth, &_builder] (const uint32_t entryID) {
    if (onRecomputedDepth) {
      (*onRecomputedDepth)(entryID, oldRootDepth, _builder->getDepth(entryID));
    }
  };

  builderToTransform->replace(rhsBuilder, rhsToLhsMapping,
                              &addNewCell, onEqualDepth, onGreaterDepth,
                              &onRecompDepthWrap);
  recomputeNext(oldRootDepth, rootLastDepth);
}

void SafePasser::finalizePass() {
  isNewEntry.clear();
  saveNext = SubnetBuilder::invalidID;
  direction = UNDEF;
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
