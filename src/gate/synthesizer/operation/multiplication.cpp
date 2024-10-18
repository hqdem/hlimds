//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "multiplication.h"
#include "addition.h"
#include "utils.h"

#include <algorithm>

namespace eda::gate::synthesizer {

model::SubnetID synthMulS(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  uint16_t sizeA = attr.getInWidth(0), sizeB = attr.getInWidth(1);
  const uint16_t outSize = attr.getOutWidth(0);

  model::Subnet::LinkList inputsForA = builder.addInputs(sizeA);
  model::Subnet::LinkList inputsForB = builder.addInputs(sizeB);
  if (sizeA < sizeB) {
    std::swap(inputsForA, inputsForB);
    std::swap(sizeA, sizeB);
  }
  if (sizeA < outSize) {
    inputsForA.resize(std::min<uint16_t>(outSize, sizeA << 1u),
                      inputsForA.back());
  }
  if (inputsForA.size() != sizeB) {
    inputsForB.resize(inputsForA.size(), inputsForB.back());
  }

  builder.addOutputs(
      synthKaratsubaMultiplyer(builder, inputsForA, inputsForB, outSize, true));
  return builder.make();
}

model::SubnetID synthMulU(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  const uint16_t sizeA = attr.getInWidth(0), sizeB = attr.getInWidth(1);
  const uint16_t outSize = attr.getOutWidth(0);

  model::Subnet::LinkList inputsForA = builder.addInputs(sizeA);
  model::Subnet::LinkList inputsForB = builder.addInputs(sizeB);
  if (sizeA < sizeB) {
    std::swap(inputsForA, inputsForB);
  }

  builder.addOutputs(
      synthKaratsubaMultiplyer(builder, inputsForA, inputsForB, outSize));
  return builder.make();
}

// synth simple one bit addition with "carry in" and "carry out"
inline std::pair<model::Subnet::Link, model::Subnet::Link>
synthFullAdder(model::SubnetBuilder &builder, const model::Subnet::Link &varA,
               const model::Subnet::Link &varB,
               const model::Subnet::Link &carryIn, bool noCarry = false) {
  model::Subnet::Link sum =
      builder.addCell(model::CellSymbol::XOR, varA, varB, carryIn);
  model::Subnet::Link carryOut =
      noCarry ? model::Subnet::Link()
              : builder.addCell(
                    model::CellSymbol::OR,
                    builder.addCell(model::CellSymbol::AND, varA, varB),
                    builder.addCell(model::CellSymbol::AND, varA, carryIn),
                    builder.addCell(model::CellSymbol::AND, carryIn, varB));
  return {sum, carryOut};
}

// default matrix multiplier for operations, size of which
// is too small to use Karatsuba multilier
// IMPORTANT: if signed is used, please,
// fill by sign bit to size "maxInputSize * 2"
model::Subnet::LinkList synthSimpleMultipyer(
    model::SubnetBuilder &builder, model::Subnet::LinkList &inputsForA,
    model::Subnet::LinkList &inputsForB, const uint16_t outSize, bool useSign) {
  model::Subnet::LinkList outputs(outSize);
  const uint16_t factSize = std::min<uint16_t>(outSize, inputsForA.size());
  const uint16_t targetSize = useSign ? inputsForA.size() : outSize;
  int16_t bitsToOutput = factSize;

  // we make a suppose, that size of first input is bigger
  model::Subnet::LinkList andOperations(factSize);
  auto iterA = inputsForA.begin(), iterB = inputsForB.begin();
  auto lambda = [&]() {
    auto andOper = builder.addCell(model::CellSymbol::AND, *iterA, *iterB);
    ++iterA;
    return andOper;
  };
  // skip the first "and" operation and add it to outputs
  outputs[0] = lambda();
  // generate other "and" operations
  std::generate(andOperations.begin(),
                andOperations.begin() + (andOperations.size() - 1u), lambda);

  uint16_t outIter = 1u, generatedSize = factSize + 1u;
  for (; outIter < factSize && (++iterB) != inputsForB.end(); ++outIter) {
    iterA = inputsForA.begin();
    // when we have already created n bits, which would be used later,
    // and we need at least n bits, we do not want to generate others
    // like targetSize if 4, and size of both digits is 4. We created
    // first 4 bits, and now we want only to use them, not to create
    // more and more. On the second step we'll create 3 bits, than - 2, etc
    if (generatedSize > targetSize) {
      --bitsToOutput;
    }
    model::Subnet::LinkList localAndOperations(bitsToOutput);
    std::generate(localAndOperations.begin(), localAndOperations.end(), lambda);

    // first half-adder for first sum in line
    auto carry = builder.addCell(model::CellSymbol::AND, andOperations[0],
                                 localAndOperations[0]);
    outputs[outIter] = builder.addCell(model::CellSymbol::XOR, andOperations[0],
                                       localAndOperations[0]);
    // generate all adders
    for (int16_t i = 1; i < bitsToOutput; ++i) {
      bool nonCarrySum = bitsToOutput < factSize && i + 1 == bitsToOutput;
      // when we add first line of adders, we don't need to use a full adder
      if (outIter == 1u && i == factSize - 1) {
        andOperations[i - 1] = builder.addCell(model::CellSymbol::XOR, carry,
                                               localAndOperations[i]);
        if (!nonCarrySum)
          carry = builder.addCell(model::CellSymbol::AND, carry,
                                  localAndOperations[i]);
        break;
      }
      // by default, we use a full adder
      auto subSum = synthFullAdder(builder, andOperations[i],
                                   localAndOperations[i], carry, nonCarrySum);
      andOperations[i - 1] = subSum.first;
      carry = subSum.second;
    }
    // and we store in the last value
    andOperations[bitsToOutput - 1] = carry;
    ++generatedSize;
  }
  // remove the last element, as by default we do not fill
  // the whole vector by operations
  if (outIter == 1u) {
    --bitsToOutput;
  }
  // anyway we want to make andOperations smaller, if we
  andOperations.resize(bitsToOutput);

  iterA = andOperations.begin();
  // by default we want to have at least
  while (outIter < targetSize && iterA != andOperations.end()) {
    outputs[outIter++] = *iterA;
    ++iterA;
  }
  if (outIter < outSize) {
    auto zeroCell = useSign ? outputs[outIter - 1u]
                            : builder.addCell(model::CellSymbol::ZERO);
    for (; outIter < outSize; ++outIter) {
      outputs[outIter] = zeroCell;
    }
  }
  return outputs;
}

model::Subnet::LinkList synthKaratsubaMultiplyer(
    model::SubnetBuilder &builder, model::Subnet::LinkList &inputsForA,
    model::Subnet::LinkList &inputsForB, const uint16_t outSize, bool useSign) {
  if (outSize == 1) {
    return {builder.addCell(model::CellSymbol::AND, inputsForA.back(),
                            inputsForB.back())};
  }
  if (inputsForA.size() <= 4) {
    return synthSimpleMultipyer(builder, inputsForA, inputsForB, outSize,
                                useSign);
  }
  const uint16_t k = inputsForA.size() / 2u + (inputsForA.size() & 1);

  model::Subnet::LinkList inputsLeftA(inputsForA.begin(),
                                      inputsForA.begin() + k);
  model::Subnet::LinkList inputsRightA(inputsForA.begin() + k,
                                       inputsForA.end());
  model::Subnet::LinkList inputsLeftB(
      inputsForB.begin(),
      inputsForB.begin() + std::min<uint16_t>(k, inputsForB.size()));
  model::Subnet::LinkList inputsRightB;

  // p1 = a1 * b1
  model::Subnet::LinkList mulLeft = synthKaratsubaMultiplyer(
      builder, inputsLeftA, inputsLeftB,
      std::min<uint16_t>(inputsLeftA.size() + inputsLeftB.size(), outSize));
  model::Subnet::LinkList mulRight;

  // a1 + a2
  model::Subnet::LinkList sumA =
      synthLadnerFisherAdd(builder, inputsLeftA, inputsRightA,
                           std::min<uint16_t>(k + 1, outSize - k));
  // basically, it's just b1, if b2 is empty
  model::Subnet::LinkList sumB = inputsLeftB;

  // if we have data for b2
  if (inputsForB.size() > k) {
    // b1 + b2
    inputsRightB = {inputsForB.begin() + k, inputsForB.end()};
    sumB = synthLadnerFisherAdd(builder, inputsLeftB, inputsRightB,
                                std::min<uint16_t>(k + 1, outSize - k));
    // p2 = a2 * b2
    mulRight = synthKaratsubaMultiplyer(
        builder, inputsRightA, inputsRightB,
        std::min<uint16_t>(inputsRightA.size() + inputsRightB.size(),
                           outSize - k),
        false);
  }
  // t = (a1 + a2) * (b1 + b2)
  model::Subnet::LinkList mulSum = synthKaratsubaMultiplyer(
      builder, sumA, sumB,
      std::min<uint16_t>(sumA.size() + sumB.size(), outSize - k));

  // -p1
  model::Subnet::LinkList negMul =
      twosComplement(builder, mulLeft, mulSum.size(), false);
  // t += (-p1)
  mulSum =
      synthLadnerFisherAdd(builder, mulSum, negMul, mulSum.size(), true, true);
  // when we have sth in p2, sub it from t
  if (inputsRightB.size()) {
    // -p2
    negMul = twosComplement(builder, mulRight, mulSum.size(), false);
    // t += (-p2)
    mulSum = synthLadnerFisherAdd(builder, mulSum, negMul, mulSum.size(), true,
                                  true);
  }
  // make shift on k bits
  // t *= 2 ^ k
  model::Subnet::LinkList zeroesK(k, builder.addCell(model::CellSymbol::ZERO));
  mulSum.insert(mulSum.begin(), zeroesK.begin(), zeroesK.end());
  uint16_t realInputSize =
      useSign ? inputsForA.size() : inputsForA.size() << 1u;
  // when output is larger than 2 ^ (2 * k), p2 part makes sense and can be
  // added
  if (mulRight.size() && outSize > (k << 1u)) {
    // as we need to shift on k elements, it's the quickest way to do it
    // without usage of more memory resources
    mulRight.insert(mulRight.begin(), zeroesK.begin(), zeroesK.end());
    mulRight.insert(mulRight.begin(), zeroesK.begin(), zeroesK.end());
    // we need only
    if (mulRight.size() > realInputSize) {
      mulRight.resize(realInputSize);
    }

    mulLeft = synthLadnerFisherAdd(
        builder, mulLeft, mulRight,
        std::min<uint16_t>(realInputSize, outSize));
  }

  model::Subnet::LinkList outputs = synthLadnerFisherAdd(
      builder, mulLeft, mulSum,
      std::min<uint16_t>(realInputSize, outSize));
  if (outputs.size() < outSize) {
    outputs.resize(outSize, useSign ? outputs.back() : zeroesK.back());
  }

  return outputs;
}

} // namespace eda::gate::synthesizer
