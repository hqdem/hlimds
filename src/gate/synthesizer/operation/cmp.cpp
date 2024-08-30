//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "cmp.h"

#include <algorithm>

namespace eda::gate::synthesizer {

// xor operations for two inputs
model::Subnet::LinkList
generatePropagate(const model::Subnet::LinkList &inputsA,
                  const model::Subnet::LinkList &inputsB,
                  model::SubnetBuilder &builder, bool inverse = false) {
  const uint16_t minSize = std::min(inputsA.size(), inputsB.size());

  model::Subnet::LinkList propagate(std::max(inputsA.size(), inputsB.size()));

  for (uint16_t i = 0u; i < minSize; ++i) {
    propagate[i] =
        builder.addCell(model::CellSymbol::XOR, inputsA[i], inputsB[i]);
    if (inverse) {
      propagate[i] = ~propagate[i];
    }
  }

  const auto &bigger = propagate.size() == inputsA.size() ? inputsA : inputsB;

  for (uint16_t i = minSize; i < propagate.size(); ++i) {
    propagate[i] = inverse ? ~bigger[i] : bigger[i];
  }

  return propagate;
}

// (more & signB) | (more & ~signA) | (signB & signA)
model::Subnet::Link generateSignedComparison(const model::Subnet::Link &more,
                                             const model::Subnet::Link &signA,
                                             const model::Subnet::Link &signB,
                                             model::SubnetBuilder &builder) {
  const auto firstAnd = builder.addCell(model::CellSymbol::AND, more, signB);
  const auto secondAnd = builder.addCell(model::CellSymbol::AND, more, ~signA);
  const auto thirdAnd = builder.addCell(model::CellSymbol::AND, ~signA, signB);

  return builder.addCellTree(model::CellSymbol::OR,
                             {firstAnd, secondAnd, thirdAnd}, 2);
}

std::pair<model::Subnet::Link, model::Subnet::LinkList>
generateComparison(model::Subnet::LinkList inputsA,
                   model::Subnet::LinkList inputsB,
                   model::SubnetBuilder &builder, bool useEquality = false) {
  int32_t minSize = inputsA.size(), maxSize = inputsB.size();
  bool biggerIsA = minSize > maxSize;
  if (biggerIsA) {
    std::swap(minSize, maxSize);
  }

  model::Subnet::LinkList propagate;

  // if we have two inputs with one bit, just skip this step, if we would not
  // use equation later and we don't need to create propagate for it
  if (inputsA.size() > !useEquality || inputsB.size() > !useEquality) {
    model::Subnet::LinkList inputsForPropA(inputsA.begin() + !useEquality,
                                           inputsA.end());
    model::Subnet::LinkList inputsForPropB(inputsB.begin() + !useEquality,
                                           inputsB.end());

    propagate =
        generatePropagate(inputsForPropA, inputsForPropB, builder, true);
  }

  // final array for "or" has same size, as "a" in "a > b"
  model::Subnet::LinkList orResult(inputsA.size());
  // used for implementing cascade "and"
  model::Subnet::Link currAnd;

  // difference between sizes
  const auto delta = maxSize - minSize;
  auto endP = propagate.end();

  // When the second number has more bits, than the first one,
  // we have such a situation. When we would implement operation
  // sum from i = n - 1 to 0 {Ai & ~Bi & prod from j = n - 1 to i {Pj}}
  // where Pj = ~(Aj ^ Bj), all Ai would be equal to zero,
  // while i >= inputsA.size(). So we can just create
  //  prod from j = n - 1 to inputsA.size() {Pj}
  if (delta && !biggerIsA) {
    // do not add one propagate, as it would be added during cycle later
    currAnd = delta > 2
                  ? builder.addCellTree(
                        model::CellSymbol::AND,
                        model::Subnet::LinkList(endP - delta + 1, endP), 2)
                  : propagate.back();
    // move iterator
    endP -= delta;
  }
  // when in same operation Bi is 0, ~Bi is 1, so we need to
  // implement firstly
  // sum from i = n - 1 to minSize {Ai & ~Bi & prod from j = n - 1 to i {Pj}}
  for (int32_t i = maxSize - 1; i >= minSize && biggerIsA; --i) {
    // first iteration
    if (i == maxSize - 1) {
      orResult[i] = inputsA[i];
    } else {
      // if we need to use "and" for implementing current propagate
      // (at the second iteration it is equal to *endP)
      if (i < maxSize - 2) {
        currAnd = builder.addCell(model::CellSymbol::AND, currAnd, *endP);
      }
      orResult[i] =
          builder.addCell(model::CellSymbol::AND, currAnd, inputsA[i]);
    }
    if (endP != propagate.begin()) {
      --endP;
    }

    // first iteration only
    if (i == maxSize - 1 && endP != propagate.end()) {
      currAnd = *endP;
    }
  }

  for (int32_t i = minSize - 1; i >= 0; --i) {
    model::Subnet::LinkList andOperation;
    andOperation.reserve(3);

    // when it is not the first iteration at all (can happen, when
    // maxSize == minSize), add new and operation for propagate
    if (i < maxSize - 1) {
      // at second iteration it would be equal to endP,
      // so there is no matter to use "and"
      if (i < maxSize - 2) {
        currAnd = builder.addCell(model::CellSymbol::AND, currAnd, *endP);
      }
      // save created "and" operation
      andOperation.push_back(currAnd);
    }

    andOperation.push_back(inputsA[i]);
    andOperation.push_back(~inputsB[i]);

    orResult[i] =
        andOperation.size() > 1
            ? builder.addCellTree(model::CellSymbol::AND, andOperation, 2)
            : andOperation.back();

    // if we can move iterator
    if (endP != propagate.begin()) {
      --endP;
    }
    // first iteration only
    if (i == maxSize - 1 && endP != propagate.end()) {
      currAnd = *endP;
    }
  }

  const auto output = orResult.size() > 1
                    ? builder.addCellTree(model::CellSymbol::OR, orResult, 2)
                    : orResult.back();

  // when we implement ">=" or "<=", we need to use propagate, created earlier
  return {output, propagate};
}

// just filling smaller array with its sign
// to make it same size, as larger one
void fillBySignum(model::SubnetBuilder::LinkList &inputsA,
                  const model::Subnet::Link signA,
                  model::SubnetBuilder::LinkList &inputsB,
                  const model::Subnet::Link signB) {
  auto &inputs = inputsA.size() > inputsB.size() ? inputsB : inputsA;
  const auto &sign = inputsA.size() > inputsB.size() ? signB : signA;
  // we can conver to 32-bit int, because max possible size of input is uin16_t
  const auto delta = abs((int32_t)inputsA.size() - (int32_t)inputsB.size());

  for (uint16_t i = 0; i < delta; ++i) {
    inputs.push_back(sign);
  }
}

// default generator for comparison for greater/less than unsigned
model::SubnetID synthNtU(const model::CellTypeAttr &attr, bool makeSwap) {

  model::SubnetBuilder builder;

  auto inputsForA = builder.addInputs(attr.getInWidth(0));
  auto inputsForB = builder.addInputs(attr.getInWidth(1));

  if (makeSwap) {
    std::swap(inputsForA, inputsForB);
  }

  builder.addOutput(generateComparison(inputsForA, inputsForB, builder).first);

  return builder.make();
}

// default generator for comparison for greater/less than signed
model::SubnetID synthNtS(const model::CellTypeAttr &attr, bool makeSwap) {
  bool useSign = attr.getInWidth(0) != 1 || attr.getInWidth(1) != 1;

  if (useSign) {
    model::SubnetBuilder builder;

    auto inputsForA = builder.addInputs(attr.getInWidth(0) - 1);
    auto signA = builder.addInput();

    auto inputsForB = builder.addInputs(attr.getInWidth(1) - 1);
    auto signB = builder.addInput();

    fillBySignum(inputsForA, signA, inputsForB, signB);

    if (makeSwap) {
      std::swap(inputsForA, inputsForB);
      std::swap(signA, signB);
    }

    const auto more = generateComparison(inputsForA, inputsForB, builder).first;

    builder.addOutput(generateSignedComparison(more, signA, signB, builder));

    return builder.make();
  }
  return synthNtU(attr, !makeSwap);
}

// default generator for comparison for greater/less than or equal unsigned
model::SubnetID synthNteU(const model::CellTypeAttr &attr, bool makeSwap) {

  model::SubnetBuilder builder;

  auto inputsForA = builder.addInputs(attr.getInWidth(0));
  auto inputsForB = builder.addInputs(attr.getInWidth(1));

  if (makeSwap) {
    std::swap(inputsForA, inputsForB);
  }

  auto [more, propagate] =
      generateComparison(inputsForA, inputsForB, builder, true);

  const auto equal =
      propagate.size() > 1
          ? builder.addCellTree(model::CellSymbol::AND, propagate, 2)
          : propagate.back();

  builder.addOutput(builder.addCell(model::CellSymbol::OR, equal, more));

  return builder.make();
}

// default generator for comparison for greater/less than or equal signed
model::SubnetID synthNteS(const model::CellTypeAttr &attr, bool makeSwap) {
  bool useSign = attr.getInWidth(0) != 1 || attr.getInWidth(1) != 1;

  if (useSign) {
    model::SubnetBuilder builder;

    auto inputsForA = builder.addInputs(attr.getInWidth(0) - 1);
    auto signA = builder.addInput();

    auto inputsForB = builder.addInputs(attr.getInWidth(1) - 1);
    auto signB = builder.addInput();

    fillBySignum(inputsForA, signA, inputsForB, signB);

    if (makeSwap) {
      std::swap(inputsForA, inputsForB);
      std::swap(signA, signB);
    }

    auto [more, propagate] =
        generateComparison(inputsForA, inputsForB, builder, true);

    // add xor for signs for adding them to other xor-s, because "xor" for signs
    // has not been created (signs where removed)
    propagate.push_back(~builder.addCell(model::CellSymbol::XOR, signA, signB));

    const auto equal =
        propagate.size() > 1
            ? builder.addCellTree(model::CellSymbol::AND, propagate, 2)
            : propagate.back();

    more = generateSignedComparison(more, signA, signB, builder);

    builder.addOutput(builder.addCell(model::CellSymbol::OR, equal, more));

    return builder.make();
  }
  return synthNteU(attr, !makeSwap);
}

// used for creating unsigned equation (with inputs generation)
model::Subnet::Link synthDefaultEqU(const model::CellTypeAttr &attr,
                                    model::SubnetBuilder &builder) {
  auto inputsForA = builder.addInputs(attr.getInWidth(0));
  auto inputsForB = builder.addInputs(attr.getInWidth(1));

  const auto propagate = generatePropagate(inputsForA, inputsForB, builder);

  return propagate.size() > 1
             ? ~builder.addCellTree(model::CellSymbol::OR, propagate, 2)
             : ~propagate.back();
}

// used for creating (fully, with inputs) signed equation
model::Subnet::Link synthDefaultEqS(const model::CellTypeAttr &attr,
                                    model::SubnetBuilder &builder) {

  auto inputsForA = builder.addInputs(attr.getInWidth(0));
  auto inputsForB = builder.addInputs(attr.getInWidth(1));

  fillBySignum(inputsForA, inputsForA.back(), inputsForB, inputsForB.back());

  const auto propagate = generatePropagate(inputsForA, inputsForB, builder);
  return propagate.size() > 1
             ? ~builder.addCellTree(model::CellSymbol::OR, propagate, 2)
             : ~propagate.back();
}

// functions for creating final subnets
model::SubnetID synthEqU(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  const auto output = synthDefaultEqU(attr, builder);
  builder.addOutput(output);

  return builder.make();
}

model::SubnetID synthEqS(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  const auto output = synthDefaultEqS(attr, builder);
  builder.addOutput(output);

  return builder.make();
}

model::SubnetID synthNeqU(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  const auto output = synthDefaultEqU(attr, builder);
  builder.addOutput(~output);

  return builder.make();
}

model::SubnetID synthNeqS(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  const auto output = synthDefaultEqS(attr, builder);
  builder.addOutput(~output);

  return builder.make();
}

model::SubnetID synthLtU(const model::CellTypeAttr &attr) {
  return synthNtU(attr, true);
}

model::SubnetID synthLtS(const model::CellTypeAttr &attr) {
  return synthNtS(attr, true);
}

model::SubnetID synthLteU(const model::CellTypeAttr &attr) {
  return synthNteU(attr, true);
}

model::SubnetID synthLteS(const model::CellTypeAttr &attr) {
  return synthNteS(attr, true);
}

model::SubnetID synthGtU(const model::CellTypeAttr &attr) {
  return synthNtU(attr, false);
}

model::SubnetID synthGtS(const model::CellTypeAttr &attr) {
  return synthNtS(attr, false);
}

model::SubnetID synthGteU(const model::CellTypeAttr &attr) {
  return synthNteU(attr, false);
}

model::SubnetID synthGteS(const model::CellTypeAttr &attr) {
  return synthNteS(attr, false);
}

} // namespace eda::gate::synthesizer
