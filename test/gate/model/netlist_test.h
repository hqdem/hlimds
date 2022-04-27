//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/netlist.h"

#include <memory>

using namespace eda::gate::model;

// (x1 | ... | xN).
std::unique_ptr<Netlist> makeOr(unsigned N,
                                Signal::List &inputs,
                                Gate::Id &outputId);
// (x1 & ... & xN).
std::unique_ptr<Netlist> makeAnd(unsigned N,
                                 Signal::List &inputs,
                                 Gate::Id &outputId);
// ~(x1 | ... | xN).
std::unique_ptr<Netlist> makeNor(unsigned N,
                                 Signal::List &inputs,
                                 Gate::Id &outputId);
// ~(x1 & ... & xN).
std::unique_ptr<Netlist> makeNand(unsigned N,
                                  Signal::List &inputs,
                                  Gate::Id &outputId);
// (~x1 | ... | ~xN).
std::unique_ptr<Netlist> makeOrn(unsigned N,
                                 Signal::List &inputs,
                                 Gate::Id &outputId);
// (~x1 & ... & ~xN).
std::unique_ptr<Netlist> makeAndn(unsigned N,
                                  Signal::List &inputs,
                                  Gate::Id &outputId);
