//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "balancer.h"

namespace eda::gate::optimizer {

template<typename Iter>
struct is_reverse_iterator : std::false_type {};

template<typename Iter>
struct is_reverse_iterator<std::reverse_iterator<Iter>>
       : std::integral_constant<bool, !is_reverse_iterator<Iter>::value> {};

void Balancer::transform(
    const std::shared_ptr<SubnetBuilder> &builder) const {
  SubnetBuilder *builderPtr = builder.get();

  for (auto it = builderPtr->begin(); it != builderPtr->end();) {
    auto saveNext = it.next();
    balanceOnEntry(*builderPtr, *it);
    it = saveNext;
  }
}

bool Balancer::canBalanceCompl(
    const SubnetBuilder &builder,
    const size_t uOpEntryID,
    const size_t dOpEntryID,
    const size_t uOpSwapInput) const {

  const auto &uCell = builder.getCell(uOpEntryID);
  const auto &dCell = builder.getCell(dOpEntryID);
  const auto uInputs = builder.getLinks(uOpEntryID);
  const auto dInputs = builder.getLinks(dOpEntryID);

  if (!uCell.isMaj() || !dCell.isMaj()) {
    return false;
  }
  if (uInputs.size() != 3 || dInputs.size() != 3) {
    return false;
  }

  const size_t uOpInput2 = uInputs[1].idx;
  const size_t dOpInput2 = dInputs[1].idx;
  const size_t dOpSwapInput = uInputs[0].idx == dOpEntryID ?
                              dInputs[0].idx : dInputs[2].idx;

  return dCell.refcount == 1 && dOpInput2 == uOpInput2 &&
      builder.getDepth(uOpSwapInput) < builder.getDepth(dOpSwapInput);
}

bool Balancer::canBalanceAssoc(
    const SubnetBuilder &builder,
    const size_t uOpEntryID,
    const size_t dOpEntryID) const {

  const auto &uOpCell = builder.getCell(uOpEntryID);
  const auto &dOpCell = builder.getCell(dOpEntryID);
  if (!uOpCell.getType().isAssociative()) {
    return false;
  }
  if (uOpCell.getTypeID() != dOpCell.getTypeID()) {
    return false;
  }
  if (dOpCell.refcount > 1) {
    return false;
  }
  return true;
}

bool Balancer::canBalance(
    const SubnetBuilder &builder,
    const size_t uOpEntryID,
    const size_t dOpEntryID,
    const size_t uOpSwapInput) const {

  return canBalanceCompl(builder, uOpEntryID, dOpEntryID, uOpSwapInput) ||
         canBalanceAssoc(builder, uOpEntryID, dOpEntryID);
}

void Balancer::balanceComplAssoc(
    SubnetBuilder &builder,
    const size_t entryID) const {
  const auto &cell = builder.getCell(entryID);
  const auto &entryInputs = builder.getLinks(entryID);
  const size_t inputEntryID0 = entryInputs[0].idx;
  const size_t InputEntryID2 = entryInputs[2].idx;
  size_t dOperEntryID;

  if (canBalance(builder, entryID, inputEntryID0, InputEntryID2) &&
      !entryInputs[0].inv) {
    dOperEntryID = inputEntryID0;
  } else if (canBalance(builder, entryID, InputEntryID2, inputEntryID0) &&
             !entryInputs[2].inv) {
    dOperEntryID = InputEntryID2;
  } else {
    return;
  }

  const auto &dOperCell = builder.getCell(dOperEntryID);
  const auto &dInputs = builder.getLinks(dOperEntryID);
  LinkList newUCellInputs;
  LinkList newDCellInputs;

  if (dOperEntryID == inputEntryID0) {
    newUCellInputs = { entryInputs[0], entryInputs[1], dInputs[0] };
    newDCellInputs = { entryInputs[2], dInputs[1], dInputs[2]     };
  } else {
    newUCellInputs = { dInputs[2], entryInputs[1], entryInputs[2] };
    newDCellInputs = { dInputs[0], dInputs[1], entryInputs[0]     };
  }

  builder.replaceCell(entryID, cell.getTypeID(), newUCellInputs, false);
  builder.replaceCell(dOperEntryID, dOperCell.getTypeID(), newDCellInputs,
      false);
}

size_t Balancer::balanceOnEntry(
    SubnetBuilder &builder,
    const size_t entryID) const {
  size_t depthBeforeBalancings = builder.getDepth(entryID);
  const auto &cell = builder.getCell(entryID);
  size_t depthBeforeBalancing;

  do {
    depthBeforeBalancing = builder.getDepth(entryID);
    if (cell.getType().isAssociative()) {
      if (cell.getType().isCommutative()) {
        balanceCommutAssoc(builder, entryID);
      } else {
        balanceAssoc(builder, entryID);
      }
    } else if (cell.isMaj()) { // Complementary associative function
      balanceComplAssoc(builder, entryID);
    }
  } while(builder.getDepth(entryID) < depthBeforeBalancing);

  assert(depthBeforeBalancing >= builder.getDepth(entryID));
  return depthBeforeBalancings - builder.getDepth(entryID);
}

template<typename Iter>
void Balancer::moveOp(
    SubnetBuilder &builder,
    const size_t entryID,
    const Iter &operIter,
    const Iter &inputsBegin,
    const Iter &inputsEnd,
    const Iter &dInputsBegin,
    const Iter &dInputsEnd) const {

  const auto &cell = builder.getCell(entryID);
  const size_t dOperEntryID = operIter->idx;
  const auto &dOperCell = builder.getCell(dOperEntryID);
  LinkList newEntryInputs;
  LinkList newDOperEntryInputs;

  for (auto inputsIt = inputsBegin; inputsIt != inputsEnd; ++inputsIt) {
    if (inputsIt != operIter) {
      newEntryInputs.push_back(*inputsIt);
    } else {
      newEntryInputs.push_back(*dInputsBegin);
      newEntryInputs.push_back(*inputsIt);

      for (auto dInputsIt = dInputsBegin + 1; dInputsIt != dInputsEnd;
           ++dInputsIt) {
        newDOperEntryInputs.push_back(*dInputsIt);
      }
      newDOperEntryInputs.push_back(*(inputsIt + 1));
      if (is_reverse_iterator<Iter>::value) {
        std::reverse(newDOperEntryInputs.begin(), newDOperEntryInputs.end());
      }
      inputsIt++;
    }
  }
  if (is_reverse_iterator<Iter>::value) {
    std::reverse(newEntryInputs.begin(), newEntryInputs.end());
  }

  builder.replaceCell(entryID, cell.getTypeID(), newEntryInputs, false);
  builder.replaceCell(dOperEntryID, dOperCell.getTypeID(), newDOperEntryInputs,
      false);
}

template<typename Iter>
void Balancer::moveOpToLim(
    SubnetBuilder &builder,
    const size_t entryID,
    Iter operIter,
    Iter inputsBegin,
    Iter inputsEnd,
    Iter dOpInputsBegin,
    Iter dOpInputsEnd) const {

  const size_t dOperEntryId = operIter->idx;
  size_t sideInput = (operIter + 1)->idx;
  const size_t curEntryDepth = builder.getDepth(entryID);

  while(operIter != (inputsEnd - 1) &&
        canBalance(builder, entryID, dOperEntryId, sideInput)) {

    if (builder.getDepth(sideInput) + 2 < curEntryDepth ||
        (builder.getDepth(sideInput) + 2 == curEntryDepth &&
         builder.getDepth(dOperEntryId) + 1 == curEntryDepth)) {

      moveOp(builder, entryID, operIter, inputsBegin, inputsEnd, dOpInputsBegin,
             dOpInputsEnd);
      LinkList newInputs = builder.getLinks(entryID);
      LinkList newDOpInputs = builder.getLinks(dOperEntryId);
      for (auto it = inputsBegin; it != inputsEnd; ++it) {
        if (is_reverse_iterator<Iter>::value) {
          (*it) = *(newInputs.rbegin() + (it - inputsBegin));
        } else {
          (*it) = *(newInputs.begin() + (it - inputsBegin));
        }
      }
      for (auto it = dOpInputsBegin; it != dOpInputsEnd; ++it) {
        if (is_reverse_iterator<Iter>::value) {
          (*it) = *(newDOpInputs.rbegin() + (it - dOpInputsBegin));
        } else {
          (*it) = *(newDOpInputs.begin() + (it - dOpInputsBegin));
        }
      }
      operIter++;
      sideInput = (operIter + 1)->idx;
    } else {
      break;
    }
  }
}

void Balancer::moveAllOpsLToLim(
    SubnetBuilder &builder,
    const size_t entryID) const {
  LinkList entryInputs = builder.getLinks(entryID);
  for (long long i = 1; i < (long long)entryInputs.size(); ++i) {
    size_t dOpEntryID = entryInputs[i].idx;
    size_t leftEntryID = entryInputs[i - 1].idx;
    if (!canBalance(builder, entryID, dOpEntryID, leftEntryID) ||
        entryInputs[i].inv) {
      continue;
    }
    LinkList dOpEntryInputs = builder.getLinks(dOpEntryID);
    for (long long j = (long long)dOpEntryInputs.size() - 2; j >= 0; --j) {
      size_t ddOpEntryInput = dOpEntryInputs[j].idx;
      size_t ddOpEntryRightInput = dOpEntryInputs[j + 1].idx;
      if (!canBalance(builder, dOpEntryID, ddOpEntryInput, ddOpEntryRightInput)
          || dOpEntryInputs[j].inv) {
        continue;
      }
      LinkList ddOpEntryInputs = builder.getLinks(ddOpEntryInput);
      moveOpToLim(builder, dOpEntryID, dOpEntryInputs.begin() + j,
                  dOpEntryInputs.begin(), dOpEntryInputs.end(),
                  ddOpEntryInputs.begin(), ddOpEntryInputs.end());
      break;
    }
    long long indexFromEnd = (long long)entryInputs.size() - i - 1;
    moveOpToLim(builder, entryID, entryInputs.rbegin() + (indexFromEnd),
                entryInputs.rbegin(), entryInputs.rend(),
                dOpEntryInputs.rbegin(), dOpEntryInputs.rend());
  }
}

void Balancer::moveAllOpsRToLim(
    SubnetBuilder &builder,
    const size_t entryID) const {
  LinkList entryInputs = builder.getLinks(entryID);
  for (long long i = (long long)entryInputs.size() - 2; i >= 0; --i) {
    const size_t dOpEntryID = entryInputs[i].idx;
    const size_t rightEntryID = entryInputs[i + 1].idx;

    if (!canBalance(builder, entryID, dOpEntryID, rightEntryID) ||
        entryInputs[i].inv) {
      continue;
    }
    LinkList dOpEntryInputs = builder.getLinks(dOpEntryID);
    for (long long j = 1; j < (long long)dOpEntryInputs.size(); ++j) {
      const size_t ddOpEntryInput = dOpEntryInputs[j].idx;
      const size_t ddOpEntryLeftInput = dOpEntryInputs[j - 1].idx;
      if (!canBalance(builder, dOpEntryID, ddOpEntryInput, ddOpEntryLeftInput)
          || dOpEntryInputs[j].inv) {
        continue;
      }
      LinkList ddOpEntryInputs = builder.getLinks(ddOpEntryInput);
      long long indexFromEnd = (long long)dOpEntryInputs.size() - j - 1;
      moveOpToLim(builder, dOpEntryID, dOpEntryInputs.rbegin() + (indexFromEnd),
                  dOpEntryInputs.rbegin(), dOpEntryInputs.rend(),
                  ddOpEntryInputs.rbegin(), ddOpEntryInputs.rend());
      break;
    }
    moveOpToLim(builder, entryID, entryInputs.begin() + i, entryInputs.begin(),
                entryInputs.end(), dOpEntryInputs.begin(),
                dOpEntryInputs.end());
  }
}

void Balancer::balanceAssoc(
    SubnetBuilder &builder,
    const size_t entryID) const {
  moveAllOpsLToLim(builder, entryID);
  moveAllOpsRToLim(builder, entryID);
}

void Balancer::balanceCommutAssoc(
    SubnetBuilder &builder,
    const size_t entryID) const {

  const auto &cell = builder.getCell(entryID);
  const auto &cellLinks = builder.getLinks(entryID);
  LinkList newCellLinks = cellLinks;
  const size_t curDepth = builder.getDepth(entryID);

  /* Adding all input gates in increasing depth order. */
  std::multimap<size_t, Link> depthLink;
  for (const auto &input : cellLinks) {
    size_t inEntryId = input.idx;
    depthLink.insert({ builder.getDepth(inEntryId), input });
  }

  if (depthLink.size() < 2) {
    return;
  }

  bool isOptimizable = false;
  for (const auto &link : cellLinks) {
    size_t linkIdx = link.idx;
    const auto &inputCell = builder.getCell(linkIdx);
    const auto &inCellLinks = builder.getLinks(linkIdx);
    LinkList newInCellLinks = inCellLinks;

    if (!canBalanceAssoc(builder, entryID, linkIdx) ||
        !inputCell.getType().isCommutative() ||
        builder.getDepth(linkIdx) + 1 != curDepth ||
        link.inv) {
      continue;
    }

    for (const auto &inLinkToSwap : inCellLinks) {
      size_t inputLinkIdx = inLinkToSwap.idx;
      if (depthLink.empty() || depthLink.begin()->first + 2 >= curDepth) {
        break;
      }
      if (builder.getDepth(inputLinkIdx) + 2 != curDepth) {
        continue;
      }

      Link linkToSwap = depthLink.begin()->second;
      const auto swapIter = std::find(newCellLinks.begin(), newCellLinks.end(),
          linkToSwap);
      const auto swapIter2 = std::find(newInCellLinks.begin(),
          newInCellLinks.end(), inLinkToSwap);

      newCellLinks.erase(swapIter);
      newInCellLinks.erase(swapIter2);
      depthLink.erase(depthLink.begin());
      newCellLinks.push_back(inLinkToSwap);
      newInCellLinks.push_back(linkToSwap);

      isOptimizable = true;
      builder.replaceCell(linkIdx, inputCell.getTypeID(), newInCellLinks,
          false);
    }
  }

  if (isOptimizable) {
    builder.replaceCell(entryID, cell.getTypeID(), newCellLinks, false);
  }
}

} // namespace eda::gate::optimizer
