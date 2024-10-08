//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "multiplexer.h"

namespace eda::gate::synthesizer {

// Binary MUX 2-to-1 (S, X, Y):
// OUT = (S == 0) ? X : Y.
static inline model::Subnet::Link addMux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::Link &x,
    const model::Subnet::Link &y) {
  return builder.addCell(model::OR,
      builder.addCell(model::AND, ~s, x),
      builder.addCell(model::AND,  s, y));
}

// Multibit MUX 2-to-1 (S, X[*], Y[*]):
// OUT[i] = (S == 0) ? X[i] : Y[i].
static inline model::Subnet::LinkList addMux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::LinkList &x) {
  assert(x.size() > 0 && !(x.size() & 1));
  model::Subnet::LinkList links(x.size() >> 1);
  for (size_t i = 0; i < links.size(); ++i) {
    links[i] = addMux2(builder, s, x[i], x[links.size() + i]);
  }
  return links;
}

// Multibit MUX 2-to-1 (S, X[*], Y[*]):
// OUT[i] = (S == 0) ? X[i] : Y[i].
model::SubnetID synthMux2(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;
  const auto s = builder.addInput();
  const auto x = builder.addInputs(attr.getInWidth(1));
  const auto y = builder.addInputs(attr.getInWidth(2));
  assert(x.size() == y.size());

  model::Subnet::LinkList out(x.size());
  for (size_t i = 0; i < out.size(); ++i) {
    out[i] = addMux2(builder, s, x[i], y[i]);
  }

  builder.addOutputs(out);
  return builder.make();
}

// Bitwise MUX (S[*], X[*], Y[*]), |S| == |X| == |Y|:
// OUT[i] = (S[i] == 0) ? X[i] : Y[i].
model::SubnetID synthBMux(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;

  const auto s = builder.addInputs(attr.getInWidth(0));
  const auto x = builder.addInputs(attr.getInWidth(1));
  const auto y = builder.addInputs(attr.getInWidth(2));
  assert(s.size() == x.size() && x.size() == y.size());

  model::Subnet::LinkList out(x.size());
  for(size_t i = 0; i < out.size(); ++i) {
    out[i] = addMux2(builder, s[i], x[i], y[i]);
  }

  builder.addOutputs(out);
  return builder.make();
}

// Multibit MUX (S[*], X[*]), |X| == |OUT| * 2^|S|:
// OUT[i] = X[INDEX(S) * |OUT| + i].
model::SubnetID synthMux(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;
  const auto s = builder.addInputs(attr.getInWidth(0));

  auto x = builder.addInputs(attr.getInWidth(1));
  for (size_t i = 0; i < s.size(); ++i) {
    x = addMux2(builder, s[i], x); 
  }
  assert(x.size() == attr.getOutWidth(0));  

  builder.addOutputs(x);
  return builder.make();
}

// Multibit DEMUX (S[*], X[*]), |OUT| == |X| * 2^|S|:
// OUT[i] = ((i / |X|) == INDEX(S)) ? X[i % |X|] : 0.
model::SubnetID synthDemux(const model::CellTypeAttr &attr) {
  // TODO:
  return model::OBJ_NULL_ID;
}

} // namespace eda::gate::synthesizer
