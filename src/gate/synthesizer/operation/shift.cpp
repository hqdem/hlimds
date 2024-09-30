//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "shift.h"

#include <algorithm>

namespace eda::gate::synthesizer {

model::SubnetID synthShlS(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  builder.addOutputs(synthDefaultShiftL(builder, attr, true));

  return builder.make();
}

model::SubnetID synthShlU(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  builder.addOutputs(synthDefaultShiftL(builder, attr));

  return builder.make();
}

model::SubnetID synthShrS(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  builder.addOutputs(
      synthDefaultShiftR(builder, attr, true));

  return builder.make();
}

model::SubnetID synthShrU(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  builder.addOutputs(synthDefaultShiftR(builder, attr));

  return builder.make();
}

void addInvertedMux(
    model::SubnetBuilder &builder,
    model::Subnet::LinkList &orOperations,
    const eda::gate::model::Subnet::Link &input,
    const model::Subnet::LinkList &andOperationsForSigned,
    bool invert = true) {  
  // ~(mux 1 or mux2 ...)
  const auto &invMux =
      (andOperationsForSigned.size() > 1)
          ? builder.addCellTree(model::CellSymbol::OR, andOperationsForSigned, 2)
          : andOperationsForSigned.back();

  orOperations.push_back(
      builder.addCell(model::CellSymbol::AND, invert ? ~invMux : invMux, input));
}

model::Subnet::LinkList synthDefaultShiftL(model::SubnetBuilder &builder,
                                           const model::CellTypeAttr &attr,
                                           bool useSign) {
  const auto sizeInput = attr.getInWidth(0), sizeMux = attr.getInWidth(1);
  const auto sizeOutput = attr.getOutWidth(0);

  model::Subnet::LinkList inputs = builder.addInputs(sizeInput);
  const model::Subnet::LinkList muxInputs = builder.addInputs(sizeMux);
  model::Subnet::LinkList outputs(sizeOutput);

  // to implement min number of mux, outputs of which would be used
  // choose min from 2^muxInputs and outputs size
  const auto muxOutputSize =
      sizeMux > 63 ? sizeOutput
                   : std::min<uint64_t>(1ull << sizeMux, sizeOutput);
  const auto maxOutSize =
      std::min<uint16_t>(muxOutputSize + inputs.size() - 1u, sizeOutput);

  // and operations for all multiplexors
  auto andOperations = synthMuxForShift(builder, muxInputs, muxOutputSize);

  auto iterAnd = andOperations.begin(), iterInput = inputs.begin();
  // number of inputs, can be used at this moment
  uint16_t used = 0u;
  // and size of window, where can we make sth
  uint16_t windowMinSize = inputs.size(), windowMaxSize = andOperations.size();

  if (windowMaxSize < windowMinSize) {
    std::swap(windowMaxSize, windowMinSize);
  }

  for (uint16_t out = 0; out < maxOutSize; ++out) {
    model::Subnet::LinkList orOperations;
    orOperations.reserve(used);

    // when we have moved through all "and" operations
    // move start position of number used in mux
    if (out >= (int16_t)andOperations.size()) {
      ++iterInput;
    }
    // same with current input pos
    if (out >= (int16_t)inputs.size()) {
      ++iterAnd;
    }

    if (out >= windowMaxSize) {
      --used;
    } else if (used < windowMinSize) {
      ++used;
    }

    for (auto inp = iterInput, andOp = iterAnd + (used - 1u);
         inp != iterInput + used; ++inp, --andOp) {
      orOperations.push_back(
          builder.addCell(model::CellSymbol::AND, *inp, *andOp));
    }

    // if sign is needed, adding "or" operation,
    // which will allow to choose between sign and other muxes
    if (useSign && out >= (int16_t)inputs.size()) {
      // when implementing signed shift, save mux "and"
      // if there is no digit to be chosen by mux,
      // it will choose sign bit, inputs.back()
      model::Subnet::LinkList andOperationsForSigned(
        andOperations.begin(), iterAnd);
      addInvertedMux(
        builder, orOperations, inputs.back(), andOperationsForSigned, false);      
    }

    // save from one-element or
    if (used > 1) {
      outputs[out] = builder.addCellTree(model::CellSymbol::OR, orOperations, 2);
    } else {
      outputs[out] = orOperations[0];
    }
  }
  // fill cells, which cannot be filled by correct velue, using zeroes
  if (maxOutSize < sizeOutput) {
    const auto &zeroCell =
        useSign ? inputs.back() : builder.addCell(model::CellSymbol::ZERO);
    for (uint16_t i = maxOutSize; i < sizeOutput; ++i) {
      outputs[i] = zeroCell;
    }
  }

  return outputs;
}

model::Subnet::LinkList synthDefaultShiftR(model::SubnetBuilder &builder,
                                           const model::CellTypeAttr &attr,
                                           bool useSign) {
  const auto sizeInput = attr.getInWidth(0), sizeMux = attr.getInWidth(1);
  const auto sizeOutput = attr.getOutWidth(0);

  const model::Subnet::LinkList inputs = builder.addInputs(sizeInput);
  const model::Subnet::LinkList muxInputs = builder.addInputs(sizeMux);
  model::Subnet::LinkList outputs(sizeOutput);

  const uint64_t muxOutSize =
      sizeMux > 63 ? inputs.size()
                   : std::min<uint64_t>(1ull << sizeMux, inputs.size());
  // and operations for all multiplexers
  const auto andOperations = synthMuxForShift(builder, muxInputs, muxOutSize);

  auto startVal = std::min<uint16_t>(inputs.size(), sizeOutput) - 1;

  // and size of window, where we can make sth
  uint16_t windowMaxSize = inputs.size(), windowMinSize = andOperations.size();

  if (windowMaxSize < windowMinSize) {
    std::swap(windowMaxSize, windowMinSize);
  }

  // how many operations should be skipped
  const uint16_t skip = std::max<int16_t>(0, inputs.size() - sizeOutput);
  // number of inputs, can be used at this moment
  uint16_t used = std::min(windowMinSize, skip);
  auto iterAnd = andOperations.begin(),
       iterInput =
           inputs.end() - std::max<int16_t>(0, skip - andOperations.size());

  for (int16_t out = startVal, count = skip; out >= 0; --out, ++count) {
    model::Subnet::LinkList orOperations;
    orOperations.reserve(used + useSign);

    // when we have moved through all "and" operations
    // move start position of number used in mux
    if (count >= (int16_t)andOperations.size()) {
      --iterInput;
    }
    if (count >= windowMaxSize) {
      --used;
    } else if (used < windowMinSize) {
      ++used;
    }

    for (auto inp = iterInput - used, andOp = iterAnd; inp != iterInput;
         ++inp, ++andOp) {
      orOperations.push_back(
          builder.addCell(model::CellSymbol::AND, *inp, *andOp));
    }

    if (useSign) {
      model::Subnet::LinkList andOperationsForSigned(
        andOperations.begin(), iterAnd + used);
      // unlike left risht, right shift requires 
      // to add inverted "and" operation for left muxes
      addInvertedMux(builder, orOperations, inputs.back(), andOperationsForSigned);      
    }

    if (orOperations.size() > 1) {
      outputs[out] = builder.addCellTree(model::CellSymbol::OR, orOperations, 2);
    } else {
      outputs[out] = orOperations[0];
    }
  }
  if (inputs.size() < sizeOutput) {
    const auto &zeroCell =
        useSign ? inputs.back() : builder.addCell(model::CellSymbol::ZERO);
    for (uint16_t i = inputs.size(); i < sizeOutput; ++i) {
      outputs[i] = zeroCell;
    }
  }

  return outputs;
}

// implement simple mux, creating targetOutputSize and operations
model::Subnet::LinkList
synthMuxForShift(model::SubnetBuilder &builder,
                 model::Subnet::LinkList muxInputs,
                 const uint64_t targetOutputsSize) {
  // We choose min. If inputs is bigger, we do need to use
  // all values for output and add some zeroes.
  // When the smallest is 2 ^ mixInputsSize, we choose it
  model::Subnet::LinkList andOperationsList(targetOutputsSize);

  for (auto &val : muxInputs) {
    val = ~val;
  }

  uint64_t iterCount = 0u;
  for (auto &iter : andOperationsList) {
    // save from one-element and
    if (muxInputs.size() > 1) {
      iter = builder.addCell(model::CellSymbol::AND, muxInputs);
    } else {
      iter = muxInputs.back();
    }

    auto prev = iterCount++;
    for (uint8_t idx = 0u; idx < muxInputs.size(); ++idx) {
      // in fact, we cannot have more than 63 muxInputs. we use idx as
      // index for bit, making a mask. When we have same idx-th bit,
      // we have 0 as a result. Else, when the bit value changed, we have 1
      // as a result, and we do the inversion
      auto mask = (1ull << idx);
      muxInputs[idx] =
          (prev & mask) ^ (iterCount & mask) ? ~muxInputs[idx] : muxInputs[idx];
    }
  }

  return andOperationsList;
}

} // namespace eda::gate::synthesizer
