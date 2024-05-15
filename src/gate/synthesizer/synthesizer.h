//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/net.h"

namespace eda::gate::synthesizer {

/// Synthesizes implementations of the soft blocks in the given net.
void synthSoftBlocks(const eda::gate::model::NetID netID);

} // namespace eda::gate::synthesizer
