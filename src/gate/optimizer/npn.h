//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstddef>
#include <cstdint>

namespace eda::gate::optimizer {

extern uint16_t npn4[];
extern uint32_t npn5[];

constexpr size_t npn4Num = 222;
constexpr size_t npn5Num = 343;

} // namespace eda::gate::optimizer
