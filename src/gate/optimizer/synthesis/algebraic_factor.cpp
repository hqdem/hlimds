//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/synthesis/algebraic_factor.h"
#include "gate/optimizer/synthesis/isop.h"

#include <unordered_set>

namespace eda::gate::optimizer::synthesis {

/// @cond ALIASES
using Link     = AlgebraicFactor::Link;
using SubnetID = AlgebraicFactor::SubnetID;
/// @endcond

SubnetID AlgebraicFactor::getSubnet(const SOP &func, size_t funcSize,
                                    uint16_t maxArity, bool inv) const {
  SubnetBuilder subnetBuilder;
  LinkList inputs = subnetBuilder.addInputs(funcSize);
  Link output = getFactoring(func, inputs, subnetBuilder, maxArity);
  subnetBuilder.addOutput(inv ? ~output : output);
  return subnetBuilder.make();
}

Link AlgebraicFactor::getFactoring(const SOP &func, const LinkList &inputs,
                                   SubnetBuilder &subnetBuilder,
                                   uint16_t maxArity) const {

  SOP div = findDivisor(func);

  if (div.empty()) {
    return synthFromSOP(func, inputs, subnetBuilder, maxArity);
  }
  SOP quo;
  SOP rem;

  divide(func, div, quo, rem, false);

  if (quo.size() == 1) {
    return getLiteralFactoring(func, quo[0], inputs, subnetBuilder, maxArity);
  }
  utils::makeCubeFree(quo);

  div = std::move(quo);
  quo.resize(0);
  rem.resize(0);

  divide(func, div, quo, rem, true);

  if (utils::cubeFree(div)) {
    Link divLink = getFactoring(div, inputs, subnetBuilder, maxArity);
    Link quoLink = getFactoring(quo, inputs, subnetBuilder, maxArity);
    if (rem.size()) {
      Link remLink = getFactoring(rem, inputs, subnetBuilder, maxArity);
      Link tmp = subnetBuilder.addCell(model::AND, divLink, quoLink);
      return ~subnetBuilder.addCell(model::AND, ~tmp, ~remLink);
    }
    return subnetBuilder.addCell(model::AND, divLink, quoLink);;
  }

  Cube common = utils::findCommonCube(quo);

  return getLiteralFactoring(func, common, inputs, subnetBuilder, maxArity);  
}

Link AlgebraicFactor::getLiteralFactoring(const SOP &func, const Cube lits,
                                          const LinkList &inputs,
                                          SubnetBuilder &subnetBuilder,
                                          uint16_t maxArity) const {
  Cube lit = utils::findBestLiteral(func, lits);
  SOP quo;
  SOP rem;
  divideByCube(func, lit, quo, rem);
  Link quoLink = getFactoring(quo, inputs, subnetBuilder, maxArity);
  if (rem.size()) {
    Link remLink = getFactoring(rem, inputs, subnetBuilder, maxArity);
    Link tmp = subnetBuilder.addCell(model::AND, quoLink,
        synthFromCube(lit, inputs, subnetBuilder, maxArity));
    return ~subnetBuilder.addCell(model::AND, ~tmp, ~remLink);
  }
  return subnetBuilder.addCell(model::AND, quoLink,
      synthFromCube(lit, inputs, subnetBuilder, maxArity));
}

AlgebraicFactor::SOP AlgebraicFactor::findDivisor(const SOP &func) const {
  if ((func.size() <= 1) || (utils::findAnyRepeatLiteral(func)._mask == 0u)) {
    return {};
  }
  return utils::findAnyLevel0Kernel(func);
}

void AlgebraicFactor::divide(const SOP &func, const SOP &div,
                             SOP &quo, SOP &rem, bool needRem) const {

  assert(func.size() >= div.size());

  if (div.size() == 1) {
    divideByCube(func, div[0], quo, rem);
    return;
  }

  std::unordered_map<Cube, size_t, kitty::hash<Cube>> funcMap;
  funcMap.reserve(func.size());
  for (size_t i{0}; i < func.size(); ++i) {
    funcMap.insert({func[i], i});
  }

  std::unordered_set<size_t> marked;
  marked.reserve(func.size());
  for (size_t i{0}; i < func.size(); ++i) {
    if (marked.count(i)) {
      continue;
    }
    size_t j{0};
    for (; j < div.size(); ++j) {
      if (utils::cubeContain(func[i], div[j])) {
        break;
      }
    }
    if (j == div.size()) {
      continue;
    }
    Cube qCube = utils::cutCube(func[i], div[j]);
    std::vector<size_t> toMark;
    toMark.reserve(div.size());
    for (size_t c = 0; c < div.size(); ++c) {
      if (c == j) {
        continue;
      }
      if (div[c]._mask & qCube._mask) {
        break;
      }
      Cube check{(qCube._bits | div[c]._bits), (qCube._mask | div[c]._mask)};
      if (funcMap.count(check)) {
        toMark.push_back(funcMap.at(check));
        assert(marked.count(toMark.back()) == 0);
      }
    }
    if (toMark.size() == (div.size() - 1)) {
      marked.insert(i);
      for (size_t q : toMark) {
        marked.insert(q);
      }
      quo.push_back(qCube);
    }
  }

  if (!needRem) {
    return;
  }
  rem.reserve(func.size() - quo.size() * div.size());
  for (size_t i{0}; i < func.size(); ++i) {
    if (marked.count(i) == 0) {
      rem.push_back(func[i]);
    }
  }
  
  assert(rem.size() == (func.size() - quo.size() * div.size()));
}

void AlgebraicFactor::divideByCube(const SOP &func, const Cube div,
                                   SOP &quo, SOP &rem) const {
  quo.reserve(func.size());
  rem.reserve(func.size());
  for (const Cube fCube : func) {
    if (utils::cubeContain(fCube, div)) {
      quo.push_back(utils::cutCube(fCube, div));
    } else {
      rem.push_back(fCube);
    }
  }
}

} // namespace eda::gate::optimizer::synthesis
