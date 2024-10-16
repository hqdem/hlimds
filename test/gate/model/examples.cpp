//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"

#include "gate/model/design.h"
#include "gate/model/generator/layer_generator.h"
#include "gate/model/generator/matrix_generator.h"
#include "gate/model/utils/subnet_truth_table.h"

namespace eda::gate::model {

/* in1   in2                          */
/* ┌─┐   ┌─┐                          */
/* └─┘─┐ └─┘─┐                        */
/* ┌─┐ |_┌─┐ |_┌─┐                    */
/* └─┘───└─┘───└─┘─┐                  */
/* in0  and4   and5|                  */
/*             ┌─┐ |_┌─┐   ┌─┐        */
/*             └─┘───└─┘───└─┘        */
/*             in3   or6   out7       */
SubnetID makeSubnet2AndOr() {
  SubnetBuilder builder;
  auto inputs = builder.addInputs(4);
  auto andCell4 = builder.addCell(CellSymbol::AND, inputs[0], inputs[1]);
  auto andCell5 = builder.addCell(CellSymbol::AND, inputs[2], andCell4);
  auto orCell6 = builder.addCell(CellSymbol::OR, andCell5, inputs[3]);
  builder.addOutput(orCell6);
  return builder.make();
}

NetID makeNet2AndOr() {
  return makeNet(makeSubnet2AndOr());
}

/* in1   in2                          */
/* ┌─┐   ┌─┐                          */
/* └─┘─┐ └─┘─┐─────┐                  */
/* ┌─┐ |_┌─┐ |_┌─┐ |                  */
/* └─┘───└─┘───└─┘ |                  */
/* in0  and4   and5|                  */
/*             ┌─┐ |_┌─┐   ┌─┐        */
/*             └─┘───└─┘───└─┘        */
/*             in3   or6   out7       */
SubnetID makeSubnet2AndOr2() {
  SubnetBuilder builder;
  auto inputs = builder.addInputs(4);
  auto andCell4 = builder.addCell(CellSymbol::AND, inputs[0], inputs[1]);
  builder.addCell(CellSymbol::AND, inputs[2], andCell4);
  auto orCell6 = builder.addCell(CellSymbol::OR, inputs[2], inputs[3]);
  builder.addOutput(orCell6);
  return builder.make();
}

NetID makeNet2AndOr2() {
  return makeNet(makeSubnet2AndOr2());
}

/* in1 ┌─┐                        */
/*     └─┘─┐ and                  */
/* in2 ┌─┐ |_┌─┐                  */
/*     └─┘───└─┘─┐ and            */
/*       in3 ┌─┐ |_┌─┐            */
/*           └─┘───└─┘─┐          */
/*       in4 ┌─┐       |          */
/*           └─┘─┐ xor |  or  out */
/*       in5 ┌─┐ |_┌─┐ |_┌─┐__┌─┐ */
/*           └─┘───└─┘───└─┘  └─┘ */
SubnetID makeSubnet3AndOrXor() {
  SubnetBuilder builder;
  Subnet::Link links[10];
  auto inputs = builder.addInputs(5);
  std::copy(inputs.begin(), inputs.end(), links);
  links[5] = builder.addCell(AND, links[0], links[1]);
  links[6] = builder.addCell(AND, links[5], links[2]);
  links[7] = builder.addCell(XOR, links[3], links[4]);
  links[8] = builder.addCell(OR, links[6], links[7]);
  links[9] = builder.addOutput(links[8]);
  return builder.make();
}

NetID makeNet3AndOrXor() {
  return makeNet(makeSubnet3AndOrXor());
}

/*             in1 ┌─┐                  */
/*                 └─┘─┐  or            */
/*             in2 ┌─┐ |_┌─┐            */
/*                 └─┘───└─┘─┐          */
/* in3 ┌─┐                   |          */
/*     └─┘─┐ xor             |          */
/* in4 ┌─┐ |_┌─┐             |          */
/*     └─┘───└─┘─┐           |          */
/* in5 ┌─┐       |           |          */
/*     └─┘─┐ and |  or   not | and  out */
/* in6 ┌─┐ |_┌─┐ |_┌─┐___┌─┐ |_┌─┐__┌─┐ */
/*     └─┘───└─┘───└─┘   └─┘───└─┘  └─┘ */
SubnetID makeSubnetXorNorAndAndOr() {
  SubnetBuilder builder;
  Subnet::Link links[13];
  auto inputs = builder.addInputs(6);
  std::copy(inputs.begin(), inputs.end(), links);
  links[6] = builder.addCell(OR, links[0], links[1]);
  links[7] = builder.addCell(XOR, links[2], links[3]);
  links[8] = builder.addCell(AND, links[4], links[5]);
  links[9] = builder.addCell(OR, links[7], links[8]);
  links[9].inv = true;
  links[10] = builder.addCell(BUF, links[9]);
  links[11] = builder.addCell(AND, links[6], links[10]);
  builder.addOutput(links[11]);
  return builder.make();
}

NetID makeNetXorNorAndAndOr() {
  return makeNet(makeSubnetXorNorAndAndOr());
}

/* in1 ┌─┐                  */
/*     └─┘───┐ xor          */
/* in2 ┌─┐___├─┐            */
/*     └─┘─┐ └─┘─┐  or  out */
/* in3 ┌─┐ |_┌─┐ |_┌─┐__┌─┐ */
/*     └─┘───└─┘───└─┘  └─┘ */
/*           xor            */
SubnetID makeSubnetXorOrXor() {
  SubnetBuilder builder;
  Subnet::Link links[7];
  auto inputs = builder.addInputs(3);
  std::copy(inputs.begin(), inputs.end(), links);
  links[3] = builder.addCell(XOR, links[0], links[1]);
  links[4] = builder.addCell(XOR, links[1], links[2]);
  links[5] = builder.addCell(OR, links[3], links[4]);
  builder.addOutput(links[5]);
  return builder.make();
}

NetID makeNetXorOrXor() {
  return makeNet(makeSubnetXorOrXor());
}

/* in           and  out */
/* ┌─┐───────┬─┌─┐──┌─┐  */
/* └─┘     ┌─┼─└─┘  └─┘  */
/* in      | │  or   out */
/* ┌─┐     | ├─┌─┐──┌─┐  */
/* └─┘─────┼─┼─└─┘  └─┘  */
/*         | │  xor  out */
/*         | └─┌─┐──┌─┐  */
/*         └───└─┘  └─┘  */
SubnetID makeSubnetAndOrXor() {
  SubnetBuilder builder;
  Subnet::Link links[8];
  auto inputs = builder.addInputs(2);
  std::copy(inputs.begin(), inputs.end(), links);
  links[2] = builder.addCell(AND, links[0], links[1]);
  links[3] = builder.addCell(OR, links[0], links[1]);
  links[4] = builder.addCell(XOR, links[0], links[1]);
  builder.addOutput(links[2]);
  builder.addOutput(links[3]);
  builder.addOutput(links[4]);
  return builder.make();
}

NetID makeNetAndOrXor() {
  return makeNet(makeSubnetAndOrXor());
}

/* in           and      and  out */
/* ┌─┐───────┬─┌─┐──┬────┌─┐──┌─┐ */
/* └─┘     ┌─┼─└─┘  |    └─┘  └─┘ */
/*         | |      |    and  out */
/*         | |      └────┌─┐──┌─┐ */
/*         | |      ┌────└─┘  └─┘ */
/* in      | │  or  |    or   out */
/* ┌─┐     | └─┌─┐──┴────┌─┐──┌─┐ */
/* └─┘─────┴───└─┘       └─┘  └─┘ */
SubnetID makeSubnet4AndOr() {
  SubnetBuilder builder;
  Subnet::Link links[10];
  auto inputs = builder.addInputs(2);
  std::copy(inputs.begin(), inputs.end(), links);
  links[2] = builder.addCell(AND, links[0], links[1]);
  links[3] = builder.addCell(OR, links[0], links[1]);
  links[4] = builder.addCell(AND, links[2]);
  links[5] = builder.addCell(AND, links[2], links[3]);
  links[6] = builder.addCell(OR, links[3]);
  builder.addOutput(links[4]);
  builder.addOutput(links[5]);
  builder.addOutput(links[6]);
  return builder.make();
}

NetID makeNet4AndOr() {
  return makeNet(makeSubnet4AndOr());
}

/*
in   lat  out
┌─┐──┌─┐──┌─┐
└─┘  └─┘  └─┘
in   lat  out
┌─┐──┌─┐──┌─┐
└─┘  └─┘  └─┘
*/
SubnetID makeSubnet2Latches() {
  SubnetBuilder builder;
  Subnet::Link links[8];
  auto inputs = builder.addInputs(2);
  std::copy(inputs.begin(), inputs.end(), links);
  links[2] = builder.addInput();
  links[3] = builder.addInput();
  builder.addOutput(links[0]);
  builder.addOutput(links[1]);
  builder.addOutput(links[2]);
  builder.addOutput(links[3]);
  return builder.make();
}

NetID makeNet2Latches() {
  return makeNet(makeSubnet2Latches());
}

/* in           and                    */
/* ┌─┐───────┬─┌─┐──┐                  */
/* └─┘     ┌─┼─└─┘  |    or   lat  out */
/*         | |      └────┌─┐──┌─┐──┌─┐ */
/*         | |      ┌────└─┘  └─┘  └─┘ */
/* in      | │  or  |                  */
/* ┌─┐     | └─┌─┐──┘                  */
/* └─┘─────┴───└─┘                     */
SubnetID makeSubnetLatch() {
  SubnetBuilder builder;
  Subnet::Link links[8];
  auto inputs = builder.addInputs(2);
  std::copy(inputs.begin(), inputs.end(), links);
  links[2] = builder.addInput();
  links[3] = builder.addCell(AND, links[0], links[1]);
  links[4] = builder.addCell(OR, links[0], links[1]);
  links[5] = builder.addCell(OR, links[3], links[4]);
  builder.addOutput(links[5]);
  builder.addOutput(links[2]);
  return builder.make();
}

NetID makeNetLatch() {
  return makeNet(makeSubnetLatch());
}

/* in          or          lat  out */
/* ┌─┐──┬──────┌─┐   and   ┌─┐──┌─┐ */
/* └─┘  |      └─┘──┌─┐────└─┘  └─┘ */
/*      | not  or  ┌└─┘──┐ lat  out */
/*      └─┌─┐──┌─┐─┘     └─┌─┐──┌─┐ */
/*        └─┘  └─┘         └─┘  └─┘ */
SubnetID makeSubnetStuckLatches() {
  SubnetBuilder builder;
  Subnet::Link links[10];
  links[0] = builder.addInput();
  links[1] = builder.addInput();
  links[2] = builder.addInput();
  links[3] = builder.addCell(OR, links[0]);
  links[4] = builder.addCell(OR, Subnet::Link(links[0].idx, true));
  links[5] = builder.addCell(AND, links[3], links[4]);
  builder.addOutput(links[5]);
  builder.addOutput(links[5]);
  builder.addOutput(links[1]);
  builder.addOutput(links[2]);
  return builder.make();
}

NetID makeNetStuckLatches() {
  return makeNet(makeSubnetStuckLatches());
}

/*
in   lat  out
┌─┐──┌─┐──┌─┐
└─┘  └─┘  └─┘
in   lat  out
┌─┐──┌─┐──┌─┐
└─┘  └─┘  └─┘
0    lat  out
┌─┐──┌─┐──┌─┐
└─┘  └─┘  └─┘
*/
SubnetID makeSubnetStuckLatch() {
  SubnetBuilder builder;
  Subnet::Link links[12];
  links[0] = builder.addInput();
  links[1] = builder.addInput();
  links[3] = builder.addInput();
  links[4] = builder.addInput();
  links[5] = builder.addInput();
  links[2] = builder.addCell(ZERO);
  builder.addOutput(links[0]);
  builder.addOutput(links[1]);
  builder.addOutput(links[2]);
  builder.addOutput(links[3]);
  builder.addOutput(links[4]);
  builder.addOutput(links[5]);
  return builder.make();
}

NetID makeNetStuckLatch() {
  return makeNet(makeSubnetStuckLatch());
}

SubnetID makeSubnetRandomMatrix(const size_t nIn,
                                const size_t nOut,
                                const size_t nCell,
                                const size_t minArity,
                                const size_t maxArity,
                                const unsigned seed) {
  const auto netID = makeNetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);

  DesignBuilder builder(netID);
  return builder.getSubnetID(0);
}

const SubnetBuilderPtr makeBuilderRandomMatrix(const size_t nIn,
                                               const size_t nOut,
                                               const size_t nCell,
                                               const size_t minArity,
                                               const size_t maxArity,
                                               const unsigned seed) {
  const auto netID = makeNetRandomMatrix(
      nIn, nOut, nCell, minArity, maxArity, seed);
  DesignBuilder builder(netID);
  return builder.getSubnetBuilder(0);
}

NetID makeNetRandomMatrix(const size_t nIn,
                          const size_t nOut,
                          const size_t nCell,
                          const size_t minArity,
                          const size_t maxArity,
                          const unsigned seed) {
  MatrixGenerator generator(
      nCell, nIn, nOut, {AND, OR, XOR, NAND, NOR, XNOR}, seed);

  generator.setFaninLim(minArity, maxArity);
  return generator.generate();
}

NetID makeTriggerNetRandomMatrix(const size_t nIn,
                                 const size_t nOut,
                                 const size_t nCell,
                                 const size_t minArity,
                                 const size_t maxArity,
                                 const unsigned seed) {
  MatrixGenerator generator(
      nCell, nIn, nOut, {AND, OR, XOR, NAND, NOR, XNOR, sDFF_pp0, sDFF_pp1,
      sDFF_pn0, sDFF_pn1, sDFF_nn0, sDFF_nn1, sDFF_pp0, sDFF_pp1}, seed);

  generator.setFaninLim(minArity, maxArity);
  return generator.generate();
}

NetID makeTriggerNetRandomLayer(const size_t nIn,
                                const size_t nOut,
                                const size_t nLayers,
                                const uint16_t layerNCellsMin,
                                const uint16_t layerNCellsMax,
                                const size_t minArity,
                                const size_t maxArity,
                                const unsigned seed) {
  LayerGenerator generator(
      nIn, nOut, {AND, OR, XOR, NAND, NOR, XNOR, sDFF_pp0, sDFF_pp1,
      sDFF_pn0, sDFF_pn1, sDFF_nn0, sDFF_nn1, sDFF_pp0, sDFF_pp1}, nLayers,
      layerNCellsMin, layerNCellsMax,  seed);

  generator.setFaninLim(minArity, maxArity);
  return generator.generate();
}

} // namespace eda::gate::model
