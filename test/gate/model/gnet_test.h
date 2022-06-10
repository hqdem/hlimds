//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"

#include <memory>

using namespace eda::gate::model;

// (x1 | ... | xN).
std::unique_ptr<GNet> makeOr(unsigned N,
                             Signal::List &inputs,
                             Gate::Id &outputId);
// (x1 & ... & xN).
std::unique_ptr<GNet> makeAnd(unsigned N,
                              Signal::List &inputs,
                              Gate::Id &outputId);
// ~(x1 | ... | xN).
std::unique_ptr<GNet> makeNor(unsigned N,
                              Signal::List &inputs,
                              Gate::Id &outputId);
// ~(x1 & ... & xN).
std::unique_ptr<GNet> makeNand(unsigned N,
                               Signal::List &inputs,
                               Gate::Id &outputId);
// (~x1 | ... | ~xN).
std::unique_ptr<GNet> makeOrn(unsigned N,
                              Signal::List &inputs,
                              Gate::Id &outputId);
// (~x1 & ... & ~xN).
std::unique_ptr<GNet> makeAndn(unsigned N,
                               Signal::List &inputs,
                               Gate::Id &outputId);
