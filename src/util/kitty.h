//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/optimizer/npndb.h"

#include "kitty/kitty.hpp"

/**
 * \brief Utility methods for kitty lib.
 */
namespace eda::utils {

template<typename TT>
eda::gate::optimizer::NPNDatabase::NPNTransformation
getTransformation(const std::tuple<TT, uint32_t, std::vector<uint8_t> > &t) {
  return eda::gate::optimizer::NPNDatabase::NPNTransformation{std::get<1>(t),
                                                              std::get<2>(t)};
}

template<typename TT> TT
getTT(const std::tuple<TT, uint32_t, std::vector<uint8_t>> &t) {
  return std::get<0>(t);
}

} // namespace eda::utils
