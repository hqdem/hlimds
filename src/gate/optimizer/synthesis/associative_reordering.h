//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "gate/model/subnetview.h"
#include "gate/optimizer/synthesizer.h"

#include <map>
#include <memory>
#include <set>

namespace eda::gate::optimizer::synthesis {

/// @cond ALIASES
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using CellSymbol = eda::gate::model::CellSymbol;
using SubnetView = eda::gate::model::SubnetView;
using SubnetID = eda::gate::model::SubnetID;
/// @endcond

/** 
 * @brief Info about SubnetBuilder or fragment of SubnetBuilder, 
 * which is analysing for optimization.
 */

struct FragmentInfo {
public:
  /// Empty constructor.
  FragmentInfo() {};

  FragmentInfo(std::shared_ptr<SubnetBuilder> builder, 
               std::vector<float> &wghts,
               size_t depth, 
               size_t arity) : 
                  
  builder(builder), weights(wghts), depth(depth), arity(arity) {}; 

  std::shared_ptr<SubnetBuilder> builder;
  std::vector<float> weights;
  size_t depth;
  size_t arity;
  std::vector<int> goodPermutation;
};

/** 
 * @brief Synthesizer based on associativity and commutativity 
 * of cone's function.
 */

class AssociativeReordering : public Synthesizer<SubnetBuilder> {
public:

/// Empty constructor.
AssociativeReordering() {};

/// Synthesizes a SubnetObject for the given builder and care specification.
virtual model::SubnetObject synthesize(const SubnetBuilder &builder,
                                       const utils::TruthTable &care,
                                       const uint16_t maxArity = -1) const {
  return synthesize(builder, maxArity);
};

/// Synthesizes a SubnetObject for the given builder.
virtual model::SubnetObject synthesize(const SubnetBuilder &builder, 
                                       uint16_t maxArity = -1) const;

private:

void dfsBuilder(const SubnetBuilder &builder, 
                size_t start, 
                std::vector<size_t> &mapInputs, 
                std::set<size_t> &negInpts) const;

std::shared_ptr<SubnetBuilder> makeBuilder(
    SubnetView &view, 
    const std::set<size_t> &negInpts = {}) const;

std::shared_ptr<SubnetBuilder> createBuilder(
    const size_t numInputs, 
    size_t depth,
    const size_t arity,
    std::vector<std::set<int>> &permut, 
    const CellSymbol cellSymbol, 
    const std::set<size_t> &negInputs = {}) const;

float getEffect(FragmentInfo &info,
                const std::vector<int> &permutation) const;

void combination(std::vector<int> &permutation,
                 float &maxEffect,
                 std::vector<size_t> &inEl,
                 size_t value,
                 std::map<float, size_t> &pos,
                 FragmentInfo &info) const;

std::vector<std::set<int>> createSet(const std::vector<int> &vec, 
                                     const size_t arity) const;

void setWeights(SubnetBuilder &newBuilder, 
                SubnetBuilder &parent) const;

void setWeights(SubnetView &view,
                SubnetBuilder &newBuilder, 
                const std::set<size_t> &negLinks = {}) const;

bool isAssociative(const SubnetBuilder &builder) const;
bool isOpen(SubnetView &view) const;

constexpr static float epsilon = 1e-7;
};
} // namespace eda::gate::optimizer::synthesis