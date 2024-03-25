//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/examples.h"
#include "gate/model2/utils/subnet_truth_table.h"

namespace eda::gate::model {
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
SubnetID make3AndOrXor() {
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
SubnetID makeXorNorAndAndOr() {
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

/* in1 ┌─┐                  */
/*     └─┘───┐ xor          */
/* in2 ┌─┐___├─┐            */
/*     └─┘─┐ └─┘─┐  or  out */
/* in3 ┌─┐ |_┌─┐ |_┌─┐__┌─┐ */
/*     └─┘───└─┘───└─┘  └─┘ */
/*           xor            */
SubnetID makeXorOrXor() {
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

} // namespace eda::gate::model
