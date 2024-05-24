//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/examples.h"

#include "gate/model/design.h"
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
  SubnetBuilder subnetBuilder;
  Subnet::LinkList inputs = subnetBuilder.addInputs(4);
  Subnet::Link andCell4 = subnetBuilder.addCell(CellSymbol::AND, 
                                          inputs[0], 
                                          inputs[1]);
  Subnet::Link andCell5 = subnetBuilder.addCell(CellSymbol::AND, 
                                          inputs[2], 
                                          andCell4);
  Subnet::Link orCell6 = subnetBuilder.addCell(CellSymbol::OR, 
                                         andCell5, 
                                         inputs[3]);
  subnetBuilder.addOutput(orCell6);
  return subnetBuilder.make();
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
  SubnetBuilder subnetBuilder;
  Subnet::LinkList inputs = subnetBuilder.addInputs(4);
  Subnet::Link andCell4 = subnetBuilder.addCell(CellSymbol::AND, 
                                          inputs[0], 
                                          inputs[1]);
  subnetBuilder.addCell(CellSymbol::AND, inputs[2], andCell4);
  Subnet::Link orCell6 = subnetBuilder.addCell(CellSymbol::OR, 
                                         inputs[2], 
                                         inputs[3]);
  subnetBuilder.addOutput(orCell6);

  return subnetBuilder.make();
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
  SubnetBuilder sb;
  Subnet::Link links[10];
  Subnet::LinkList inputs = sb.addInputs(5);
  std::copy(inputs.begin(), inputs.end(), links);
  links[5] = sb.addCell(AND, links[0], links[1]);
  links[6] = sb.addCell(AND, links[5], links[2]);
  links[7] = sb.addCell(XOR, links[3], links[4]);
  links[8] = sb.addCell(OR, links[6], links[7]);
  links[9] = sb.addOutput(links[8]);
  return sb.make();
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
  SubnetBuilder sb;
  Subnet::Link links[13];
  Subnet::LinkList inp = sb.addInputs(6);
  std::copy(inp.begin(), inp.end(), links);
  links[6] = sb.addCell(OR, links[0], links[1]);
  links[7] = sb.addCell(XOR, links[2], links[3]);
  links[8] = sb.addCell(AND, links[4], links[5]);
  links[9] = sb.addCell(OR, links[7], links[8]);
  links[9].inv = true;
  links[10] = sb.addCell(BUF, links[9]);
  links[11] = sb.addCell(AND, links[6], links[10]);
  sb.addOutput(links[11]);
  return sb.make();
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
  SubnetBuilder sb;
  Subnet::Link links[7];
  Subnet::LinkList inp = sb.addInputs(3);
  std::copy(inp.begin(), inp.end(), links);
  links[3] = sb.addCell(XOR, links[0], links[1]);
  links[4] = sb.addCell(XOR, links[1], links[2]);
  links[5] = sb.addCell(OR, links[3], links[4]);
  sb.addOutput(links[5]);
  return sb.make();
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
  SubnetBuilder sb;
  Subnet::Link links[8];
  Subnet::LinkList inp = sb.addInputs(2);
  std::copy(inp.begin(), inp.end(), links);
  links[2] = sb.addCell(AND, links[0], links[1]);
  links[3] = sb.addCell(OR, links[0], links[1]);
  links[4] = sb.addCell(XOR, links[0], links[1]);
  sb.addOutput(links[2]);
  sb.addOutput(links[3]);
  sb.addOutput(links[4]);
  return sb.make();
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
  SubnetBuilder sb;
  Subnet::Link links[10];
  Subnet::LinkList inp = sb.addInputs(2);
  std::copy(inp.begin(), inp.end(), links);
  links[2] = sb.addCell(AND, links[0], links[1]);
  links[3] = sb.addCell(OR, links[0], links[1]);
  links[4] = sb.addCell(AND, links[2]);
  links[5] = sb.addCell(AND, links[2], links[3]);
  links[6] = sb.addCell(OR, links[3]);
  sb.addOutput(links[4]);
  sb.addOutput(links[5]);
  sb.addOutput(links[6]);
  return sb.make();
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
  SubnetBuilder sb;
  Subnet::Link links[8];
  Subnet::LinkList inp = sb.addInputs(2);
  std::copy(inp.begin(), inp.end(), links);
  links[2] = sb.addInput(0);
  links[3] = sb.addInput(1);
  sb.addOutput(links[0], 0);
  sb.addOutput(links[1], 1);
  sb.addOutput(links[2]);
  sb.addOutput(links[3]);
  return sb.make();
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
  SubnetBuilder sb;
  Subnet::Link links[8];
  Subnet::LinkList inp = sb.addInputs(2);
  std::copy(inp.begin(), inp.end(), links);
  links[2] = sb.addInput(0);
  links[3] = sb.addCell(AND, links[0], links[1]);
  links[4] = sb.addCell(OR, links[0], links[1]);
  links[5] = sb.addCell(OR, links[3], links[4]);
  sb.addOutput(links[5], 0);
  sb.addOutput(links[2]);
  return sb.make();
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
  SubnetBuilder sb;
  Subnet::Link links[10];
  links[0] = sb.addInput();
  links[1] = sb.addInput(0);
  links[2] = sb.addInput(1);
  links[3] = sb.addCell(OR, links[0]);
  links[4] = sb.addCell(OR, Subnet::Link(links[0].idx, true));
  links[5] = sb.addCell(AND, links[3], links[4]);
  sb.addOutput(links[5], 0);
  sb.addOutput(links[5], 1);
  sb.addOutput(links[1]);
  sb.addOutput(links[2]);
  return sb.make();
}

NetID makeNetStuchLatches() {
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
  SubnetBuilder sb;
  Subnet::Link links[12];
  links[0] = sb.addInput();
  links[1] = sb.addInput();
  links[3] = sb.addInput(0);
  links[4] = sb.addInput(1);
  links[5] = sb.addInput(2);
  links[2] = sb.addCell(ZERO);
  sb.addOutput(links[0], 0);
  sb.addOutput(links[1], 1);
  sb.addOutput(links[2], 2);
  sb.addOutput(links[3]);
  sb.addOutput(links[4]);
  sb.addOutput(links[5]);
  return sb.make();
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

NetID makeNetRandomMatrix(const size_t nIn,
                          const size_t nOut,
                          const size_t nCell,
                          const size_t minArity,
                          const size_t maxArity,
                          const unsigned seed) {
  MatrixGenerator generator(
      nCell, nIn, nOut, {AND, OR, XOR, NAND, NOR, XNOR}, seed);

  generator.setFaninLim(minArity, maxArity);
  return  generator.generate();
}

} // namespace eda::gate::model
