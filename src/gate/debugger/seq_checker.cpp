//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/debugger/seq_checker.h"

#include <iostream>
#include <set>
#include <vector>

namespace eda::gate::debugger {

CheckerResult SeqChecker::isSat(const model::Subnet &subnet) const {
  const auto &miter = seqSweep(subnet);

  const auto &sweptMiter =
      structuralRegisterSweep(miter, nsimulate, setSeed, seed);

  if (this->getResult(sweptMiter).equal()) {
    return CheckerResult::EQUAL;
  }

  // TODO: forward retiming

  // TODO: partRegisterCorrespondence(seqMiter);

  // TODO: combSatSweep(seqMiter);

  return this->getResult(sweptMiter);
}

void SeqChecker::setSimulateTries(int tries) { nsimulate = tries; }

void SeqChecker::setSimulateSeed(uint32_t s) {
  nsimulate = 1;
  seed = s;
  setSeed = true;
}

CheckerResult SeqChecker::getResult(const model::Subnet &subnet) const {
  if (subnet.size() == 2 && subnet.getInNum() == 0 && subnet.getOutNum() == 1 &&
      subnet.getEntries()[0].cell.getSymbol() == model::ZERO) {
    return CheckerResult::EQUAL;
  }
  return CheckerResult::NOTEQUAL;
}

size_t addCellToBuilder(model::SubnetBuilder &builder,
                        const model::Subnet::Cell &cell,
                        const model::Subnet::LinkList &newLinks) {
  if (cell.isFlipFlop()) {
    if (cell.isIn()) {
      return builder.addInput(cell.flipFlopID).idx;
    }
    assert(newLinks.size() == 1);
    return builder.addOutput(newLinks[0], cell.flipFlopID).idx;
  }
  return builder.addCell(cell.getSymbol(), newLinks).idx;
}

const model::Subnet &seqSweep(const model::Subnet &miter) {
  std::set<size_t> usefulCells;
  std::vector<size_t> idxs;

  // get outputs
  for (size_t i = (miter.size() - miter.getOutNum()); i < miter.size(); ++i) {
    usefulCells.insert(i);
    idxs.push_back(i);
  }

  // get other cells
  size_t left = 0;
  while (left < idxs.size()) {
    model::Subnet::LinkList linkList = miter.getLinks(idxs[left]);
    for (size_t i = 0; i < linkList.size(); ++i) {
      if (usefulCells.find(linkList[i].idx) == usefulCells.end()) {
        usefulCells.insert(linkList[i].idx);
        idxs.push_back(linkList[i].idx);
      }
    }
    ++left;
  }

  std::sort(idxs.begin(), idxs.end());

  model::SubnetBuilder builder;
  auto entries = miter.getEntries();

  BaseChecker::CellToCell newIdx;
  for (const auto &id : idxs) {
    model::Subnet::LinkList oldLinks = miter.getLinks(id);
    model::Subnet::LinkList newLinks;
    for (auto &link : oldLinks) {
      newLinks.emplace_back(newIdx[link.idx], bool(link.inv));
    }

    const auto &cell = entries[id].cell;
    newIdx[id] = addCellToBuilder(builder, cell, newLinks);
  }

  return model::Subnet::get(builder.make());
}

const model::Subnet &merge(const model::Subnet &subnet,
                           std::unordered_map<size_t, std::vector<size_t>> &classes,
                           bool speculative) {
  BaseChecker::CellToCell maps;
  for (const auto &eq : classes) {
    for (const auto &id : eq.second) {
      maps[id] = eq.first;
    }
  }

  model::SubnetBuilder builder;

  auto entries = subnet.getEntries();
  size_t i = 0;
  BaseChecker::CellToCell newIdx;
  while (i < entries.size()) {
    model::Subnet::LinkList oldLinks = subnet.getLinks(i);
    model::Subnet::LinkList newLinks;
    for (auto &link : oldLinks) {
      auto idx = link.idx;
      idx = (maps.find(idx) == maps.end()) ? idx : maps[idx];
      newLinks.emplace_back(newIdx[idx], bool(link.inv));
    }

    const auto &cell = entries[i].cell;
    if (!(cell.isFlipFlop() && cell.isOut() && maps.find(i) != maps.end())) {
      newIdx[i] = addCellToBuilder(builder, cell, newLinks);
    }
    i += 1 + cell.more;
  }
  const auto notSweppedId = builder.make();
  if (speculative) {
    return model::Subnet::get(notSweppedId);
  }
  return seqSweep(model::Subnet::get(notSweppedId));
}

const model::Subnet &merge(const model::Subnet &subnet,
                    model::CellSymbol symbol,
                    const std::vector<size_t> &ids) {
  assert(symbol == model::ZERO || symbol == model::ONE);
  auto entries = subnet.getEntries();

  model::SubnetBuilder builder;
  std::unordered_map<size_t, size_t> newIdx;
  size_t id;
  for (id = 0; id < subnet.getInNum(); ++id) {
    if (entries[id].cell.isFlipFlop()) {
      newIdx[id] = builder.addInput(entries[id].cell.flipFlopID).idx;
    } else {
      newIdx[id] = builder.addInput().idx;
    }
  }
  size_t repl = builder.addCell(symbol).idx;

  while (id < entries.size()) {
    model::Subnet::LinkList oldLinks = subnet.getLinks(id);
    model::Subnet::LinkList newLinks;
    for (auto &link : oldLinks) {
      size_t idx = link.idx;
      if (std::find(ids.begin(), ids.end(), idx) != std::end(ids)) {
        idx = repl;
      } else {
        idx = newIdx[idx];
      }
      newLinks.emplace_back(idx, bool(link.inv));
    }

    const auto &cell = entries[id].cell;
    if (!(cell.isFlipFlop() && cell.isOut() &&
          std::find(ids.begin(), ids.end(), id) != std::end(ids))) {
      newIdx[id] = addCellToBuilder(builder, cell, newLinks);
    }
    id += 1 + cell.more;
  }

  return seqSweep(model::Subnet::get(builder.make()));
}

void swapFlipsValues(const simulator::Simulator::DataVector &vals1,
                     simulator::Simulator::DataVector &vals2,
                     const std::vector<std::pair<size_t, size_t>> &pairs) {
  for (const auto &p : pairs) {
    vals2[p.second] = vals1[p.first];
  }
}

void getFlipsIds(
    const model::Subnet &subnet,
    std::unordered_map<uint32_t, std::pair<size_t, size_t>> &flips) {
  auto entries = subnet.getEntries();
  for (size_t i = 0; i < subnet.getInNum(); ++i) {
    if (entries[i].cell.isFlipFlop()) {
      flips[entries[i].cell.flipFlopID].first = i;
    }
  }
  for (size_t i = 0; i < subnet.getOutNum(); ++i) {
    if (entries[subnet.size() - subnet.getOutNum() + i].cell.isFlipFlop()) {
      const auto &cell = entries[subnet.size() - subnet.getOutNum() + i].cell;
      flips[cell.flipFlopID].second = subnet.size() - subnet.getOutNum() + i;
    }
  }
}

const model::Subnet &structuralRegisterSweep(
    const model::Subnet &subnet, const int nsimulate, bool setSeed, uint32_t seed) {
  std::unordered_map<size_t, std::vector<size_t>> classes;
  auto entr = subnet.getEntries();

  // get flips ids
  std::unordered_map<uint32_t, std::pair<size_t, size_t>> flips;
  getFlipsIds(subnet, flips);

  // find flips with equal ins
  std::unordered_map<size_t, std::vector<size_t>> equal;
  size_t id;
  for (size_t i = 0; i < subnet.getOutNum(); ++i) {
    id = subnet.size() - subnet.getOutNum() + i;
    model::Subnet::LinkList links = subnet.getLinks(id);
    equal[links[0].idx].push_back(id);
  }

  for (const auto &it : equal) {
    if (it.second.size() > 1) {
      size_t repr = it.second[0];
      std::vector<size_t> copies(it.second.begin() + 1, it.second.end());
      classes[repr] = copies;

      repr = flips[entr[repr].cell.flipFlopID].first;
      for (size_t i = 0; i < copies.size(); ++i) {
        copies[i] = flips[entr[copies[i]].cell.flipFlopID].first;
      }
      classes[repr] = copies;
    }
  }

  const auto &uniq = merge(subnet, classes);
  classes.clear();

  flips.clear();
  getFlipsIds(uniq, flips);

  simulator::Simulator simulator(uniq);
  simulator::Simulator::DataVector values(uniq.getInNum());
  std::set<size_t> stuckZero, stuckOne;
  size_t tries = 1;

  for (const auto &ff : flips) {
    stuckZero.insert(ff.second.first);
    stuckZero.insert(ff.second.second);
    stuckOne.insert(ff.second.first);
    stuckOne.insert(ff.second.second);
  }

  std::mt19937 gen;
  if (setSeed) {
    gen.seed(seed);
  } else {
    std::random_device rd;
    gen.seed(rd());
  }
  std::uniform_int_distribution<uint64_t> dis;

  for (size_t i = 0; i < tries; ++i) {
    for (size_t in = 0; in < uniq.getInNum(); ++in) {
      values[in] = dis(gen);
    }
    simulator.simulate(values);

    for (const auto &it : flips) {
      if (simulator.getValue(uniq.getLink(it.second.second, 0))) {
        stuckZero.erase(it.second.first);
        stuckZero.erase(it.second.second);
      }

      if ((simulator.getValue(uniq.getLink(it.second.second, 0)) + 1)) {
        stuckOne.erase(it.second.first);
        stuckOne.erase(it.second.second);
      }
    }
  }

  const auto &withoutZero =
      merge(uniq, model::ZERO,
            std::vector<size_t>(stuckZero.begin(), stuckZero.end()));
  return merge(withoutZero, model::ONE,
               std::vector<size_t>(stuckOne.begin(), stuckOne.end()));
}

} // namespace eda::gate::debugger
