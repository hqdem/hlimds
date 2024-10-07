//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"

namespace eda::gate::synthesizer {

model::SubnetID synthAddS(const model::CellTypeAttr &attr);
model::SubnetID synthAddU(const model::CellTypeAttr &attr);

model::SubnetID synthSubS(const model::CellTypeAttr &attr);
model::SubnetID synthSubU(const model::CellTypeAttr &attr);

// As an explanation, here is a Verilog implementation of the 4-bit adder.
// Other information can be found in link below.

// Read more (in Russian):
// https://cyberleninka.ru/article/n/issledovanie-i-modifikatsiya-mnogorazryadnogo-parallelno-prefiksnogo-summatora/viewer

/*
module summator_4(
  input [3:0] a, b,
  input carry,
  output [3:0] S,
  output P
);

  wire p0 = a[0] ^ b[0];
  wire g0 = a[0] & b[0];
  wire p1 = a[1] ^ b[1];
  wire g1 = a[1] & b[1];
  wire p2 = a[2] ^ b[2];
  wire g2 = a[2] & b[2];
  wire p3 = a[3] ^ b[3];
  wire g3 = a[3] & b[3];

  wire g4 = g0 | carry & p0;

  wire g5 = g2 | g1 & p2;
  wire p5 = p2 &  p1;

  wire g6 = g5 | g4 & p5;

  wire g7 = g1 | g4 & p1;

  assign S[0] = carry ^ p0;

  assign S[1] = g4 ^ p1;

  assign S[2] = g7 ^ p2;

  assign P = g3 | g6 & p3;
  assign S[3] = g6 ^ p3;

endmodule
*/
model::Subnet::LinkList synthLadnerFisherAdd(model::SubnetBuilder &builder,
                                             model::Subnet::LinkList inputsForA,
                                             model::Subnet::LinkList inputsForB,
                                             const uint16_t outSize,
                                             bool useSign = false,
                                             bool isUnsignedSub = false);

} // namespace eda::gate::synthesizer
