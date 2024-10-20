//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "util/npn_transformation.h"

#include <kitty/kitty.hpp>

#include <cmath>
#include <vector>

/**
 * \brief Utility methods for kitty lib.
 */
namespace eda::util {

inline bool isZero(const kitty::dynamic_truth_table &tt) {
  return kitty::is_const0(tt);
}

inline bool isOne(const kitty::dynamic_truth_table &tt) {
  return kitty::is_const0(~tt);
}

inline bool isConst(const kitty::dynamic_truth_table &tt, bool &value) {
  return (value = isOne(tt)) || isZero(tt);
}

kitty::dynamic_truth_table toTT(uint64_t x);

template<typename TT>
NpnTransformation getTransformation(const std::tuple<TT, uint32_t,
                                    std::vector<uint8_t> > &t) {
  return NpnTransformation{std::get<1>(t), std::get<2>(t)};
}

template<typename TT> TT getTT(const std::tuple<TT, uint32_t,
                               std::vector<uint8_t>> &t) {
  return std::get<0>(t);
}

// FIXME: Return SubnetObject.
gate::model::SubnetID npnTransform(const gate::model::Subnet &subnet,
                                   const NpnTransformation &t,
                                   uint8_t nInUsed = -1);

//===----------------------------------------------------------------------===//
// SOP operations
//===----------------------------------------------------------------------===//

#define FIRST_ONE_BIT(integer)\
    integer - (integer & (integer - 1));

/// @cond ALIASES
using CellSymbol    = eda::gate::model::CellSymbol;
using Cube          = kitty::cube;
using KittyTT       = kitty::dynamic_truth_table;
using Link          = eda::gate::model::Subnet::Link;
using LinkList      = eda::gate::model::Subnet::LinkList;
using SOP           = std::vector<Cube>;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
/// @endcond

/// Finds any level 0 kernel for the SOP.
SOP findAnyLevel0Kernel(const SOP &sop);

/// Finds any literal that repeats more than once in the SOP.
Cube findAnyRepeatLiteral(const SOP &sop);

/// Finds the quotient of dividing the SOP by the literal.
SOP findDivideByLiteralQuotient(const SOP &sop, const Cube lit);

/// Finds a cube that is included in all cubes of the SOP.
Cube findCommonCube(const SOP &sop);

/// Checks that there is no common cube in the SOP.
bool cubeFree(const SOP &sop);

/// Deletes a cube that is contained in all cubes of the SOP.
void makeCubeFree(SOP &sop);

/// Finds among the literals of the cube the one that appears more often
/// in the SOP.
Cube findBestLiteral(const SOP &sop, Cube cube);

/// Checks that the cube contains the literal.
bool cubeHasLiteral(const Cube cube, const Cube lit);

/// Checks that the large cube includes all the elements of the small cube.
bool cubeContain(Cube large, Cube small);

/// Removes all literals of a small cube from a large cube.
Cube cutCube(Cube large, Cube small);

KittyTT generateConstTT(size_t numVars, bool on = true);

} // namespace eda::util
