//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/synthesizer/synthesizer_add.h"

#include <cassert>

namespace eda::gate::synthesizer {

inline static void checkSignature(const model::CellTypeAttr &attr) {
  assert(attr.nInPort == 2);
  assert(attr.nOutPort == 1);
}

model::SubnetID synthAdd(const model::CellTypeAttr &attr) {
  checkSignature(attr);

  model::SubnetBuilder builder;

  const auto sizeA = attr.width[0], sizeB = attr.width[1];
  const auto outSize = attr.width[2];

  model::Subnet::LinkList inputsForA = builder.addInputs(sizeA);
  model::Subnet::LinkList inputsForB = builder.addInputs(sizeB);

  builder.addOutputs(
      synthLadnerFisherAdd(builder, inputsForA, inputsForB, outSize));

  return builder.make();
}

model::SubnetID synthSub(const model::CellTypeAttr &attr) {
  checkSignature(attr);

  model::SubnetBuilder builder;

  const auto sizeA = attr.width[0], sizeB = attr.width[1];
  const auto outSize = attr.width[2];

  model::Subnet::LinkList inputsForA = builder.addInputs(sizeA);
  model::Subnet::LinkList inputsForB = builder.addInputs(sizeB);

  inputsForB =
      convertToTwosComplementCode(builder, inputsForB, std::max(sizeA, sizeB));

  builder.addOutputs(
      synthLadnerFisherAdd(builder, inputsForA, inputsForB, outSize, true));

  return builder.make();
}

model::Subnet::LinkList
convertToTwosComplementCode(model::SubnetBuilder &builder,
                            const model::Subnet::LinkList &inputsForA,
                            const uint16_t targetSize,
                            bool usedForSub) {
  model::Subnet::LinkList inversed(targetSize);
  const auto one = builder.addCell(model::CellSymbol::ONE);

  for (size_t i = 0; i < inputsForA.size(); ++i) {
    inversed[i] = ~inputsForA[i];
  }

  for (size_t i = inputsForA.size(); i < targetSize; ++i) {
    inversed[i] = one;
  }


  // return digit been inversed and for wich was added 1
  return synthLadnerFisherAdd(builder, inversed, {one}, targetSize, usedForSub);
}

// well, why builder is pointer. Such construction shows obviously,
// that builder would be changed. As we need to use "&", we would not
// forget about it. Why we do not use const links? Because we need
// to swap inputs. It is more usefull, when you exactly know, that
// one is bigger than another and it is possible to make some easy
// optimizations for this adder
model::Subnet::LinkList synthLadnerFisherAdd(model::SubnetBuilder &builder,
                                             model::Subnet::LinkList inputsForA,
                                             model::Subnet::LinkList inputsForB,
                                             const uint16_t outSize,
                                             bool usedForSub) {
  uint16_t sizeA = inputsForA.size(), sizeB = inputsForB.size();

  // the smallest is always in sizeB
  if (sizeA < sizeB) {
    std::swap(sizeA, sizeB);
    std::swap(inputsForA, inputsForB);
  }

  const uint16_t outSizeA = std::min<uint16_t>(outSize, sizeA);
  // width of output cannot be equal to zero
  assert(outSizeA > 0);
  const uint16_t outSizeB = std::min<uint16_t>(outSizeA, sizeB);

  // we need to save P values, because we will use them in the end
  model::Subnet::LinkList startOutputsP(sizeA);
  model::Subnet::LinkList gOutputs(sizeA);
  // used for gOutputs. When "i" value for gOutput is not defined
  // isNotZero[i] is false. Using it we can save us from adding
  // not used operations
  std::vector<bool> isNotZero(sizeA);

  // here is "xor" and "and" between each bit of both numbers
  // at this moment we add them only where it is necessary (where are 2 bits)

  for (uint16_t i = 0; i < outSizeB; ++i) {
    // from verilog-like [n:0] to c++ [0:n]
    startOutputsP[i] =
        builder.addCell(model::CellSymbol::XOR, inputsForA[i], inputsForB[i]);
    gOutputs[i] =
        builder.addCell(model::CellSymbol::AND, inputsForA[i], inputsForB[i]);
    isNotZero[i] = true;
  }

  // here we add left cells - we need to build tree, but not
  // all operations are necessary
  for (uint16_t i = outSizeB; i < outSizeA; ++i) {
    startOutputsP[i] = inputsForA[i];
    // mark this cells as Zeroes
    isNotZero[i] = false;
  }

  // making a copy from P outputs
  model::Subnet::LinkList pOutputs(startOutputsP);

  // firstly we move on the first level on 2 steps to further
  // links, then 4, etc
  // cycle for tree
  for (uint16_t basicStep = 1u; basicStep < outSizeA; basicStep <<= 1) {
    uint16_t delta = basicStep << 1;

    // here we select batch, with size delta and put index in its center
    for (uint16_t batch = basicStep - 1u; batch <= outSizeA; batch += delta) {
      // skip first iteration
      // we need it only if we have carry input
      if (!batch) {
        continue;
      }
      // if we would sub 1, when we are at the 0 index, we'll have problems
      // with indices. When > 1u, we can sub 1.
      model::Subnet::Link parentP = pOutputs[batch - 1u];
      model::Subnet::Link parentG = gOutputs[batch - 1u];

      // set child index to value at max possible  pos (if we choose
      // outSizeA - 1, we should be sure, that will set index to correct pos,
      // when outSizeA % 2 == 0, we need to sub 1)
      uint16_t pos =
          std::min(basicStep - 1u + batch, outSizeA - 1u - (!(outSizeA & 1)));

      // we start from the end and move to middle with step 2.
      // we are sure, that the "child" value would be smaller than 2 ^ 15 - 1,
      // as such a bits' number is not correct in fact. But if something will
      // change, it would be better to be prepared
      for (int32_t child = pos; child >= batch; child -= 2) {
        model::Subnet::Link childP = pOutputs[child];
        model::Subnet::Link childG = gOutputs[child];

        // wire p_next = p_child & p_parent;
        // if this is not first iteration
        // we add "black cell"
        if ((batch + 1u) ^ basicStep) {
          pOutputs[child] =
              builder.addCell(model::CellSymbol::AND, childP, parentP);
        }

        // if parentG is not zero, add "AND"
        if (isNotZero[batch - 1u]) {
          // wire g_next = g_child | p_child & g_parent;
          gOutputs[child] =
              builder.addCell(model::CellSymbol::AND, childP, parentG);
          // at the beginning, when we have not equal size of inputs
          // and childG would be always a zero, so we do not need to add "OR"
          if (isNotZero[child]) {
            gOutputs[child] =
                builder.addCell(model::CellSymbol::OR, childG, gOutputs[child]);
          }
          isNotZero[child] = true;
        }
      }
    }
  }

  for (uint16_t pos = 1; pos < outSizeA; pos += 2) {
    // gOutputs in this case is moved on 1 element
    // like 1 for gOutputs is 0 for pOutputs
    auto childG = gOutputs[pos];
    // if parentG is not Zero, add "AND"
    if (isNotZero[pos - 1]) {
      gOutputs[pos] = builder.addCell(model::CellSymbol::AND, pOutputs[pos],
                                      gOutputs[pos - 1]);
      // if current pos is still in sizeB borders, we need to add "OR" oper
      if (isNotZero[pos]) {
        gOutputs[pos] =
            builder.addCell(model::CellSymbol::OR, childG, gOutputs[pos]);
      }

      isNotZero[pos] = true;
    }
  }

  // when we need to add some more bits, we reserve places for them
  model::Subnet::LinkList outputGates;
  outputGates.reserve(outSize);

  // at first bit, as we have no carry in, we can just add it without "XOR"
  outputGates.push_back(startOutputsP[0]);

  // here we create sum. As in verilog it should be [n:0],
  // we should add bits in a reverse way
  for (uint16_t i = 1; i < outSizeA; ++i) {
    const model::Subnet::Link sum =
        isNotZero[i - 1] ? builder.addCell(model::CellSymbol::XOR,
                                           gOutputs[i - 1], startOutputsP[i])
                         : startOutputsP[i];
    outputGates.push_back(sum);
  }

  // Here we add carry. We do not need to add carry out etc if we are
  // implementing sub. When we need to implement sub, we need to duplicate elder
  // bit, and at this moment it hasn't been generated yet
  if (outSize > sizeA) {
    model::Subnet::Link zeroCell;

    // if we need zeroCell for carry out or for filling output
    if ((!isNotZero[gOutputs.size() - 1] || outSize > sizeA + 1) &&
        !usedForSub) {
      zeroCell = builder.addCell(model::CellSymbol::ZERO);
    }
    // if we need to add something to output and we have signed digit
    else if (usedForSub) {
      zeroCell = outputGates.back();
    }

    // add carry out. If it was not generated, set zero as output value
    // if we make sub, we add zeroCell anyway
    outputGates.push_back(isNotZero[gOutputs.size() - 1] && !usedForSub
                              ? gOutputs.back()
                              : zeroCell);

    // if we would need zeroCell, create and add it where necessary
    for (uint16_t i = sizeA + 1u; i < outSize; ++i) {
      outputGates.push_back(zeroCell);
    }
  }

  return outputGates;
}

} // namespace eda::gate::synthesizer
