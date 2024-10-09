//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "multiplexer.h"

#include <tuple>

namespace eda::gate::synthesizer {

// Binary MUX 2-to-1 (S, X, Y):
// OUT = (S == 0) ? X : Y.
model::Subnet::Link addMux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::Link &x,
    const model::Subnet::Link &y) {
  return builder.addCell(model::OR,
      builder.addCell(model::AND, ~s, x),
      builder.addCell(model::AND,  s, y));
}

// Multibit MUX 2-to-1 (S, X[*]):
// OUT[i] = (S == 0) ? X[i] : X[|X|/2 + i].
model::Subnet::LinkList addMux2(
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
model::Subnet::LinkList addMux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::LinkList &x,
    const model::Subnet::LinkList &y) {
  assert(x.size() == y.size());
  model::Subnet::LinkList links(x.size());
  for (size_t i = 0; i < links.size(); ++i) {
    links[i] = addMux2(builder, s, x[i], y[i]);
  }
  return links;
}

// Binary DEMUX 1-to-2 (S, X):
// OUT[s] = (S == s) ? X: 0, s=0,1.
std::pair<model::Subnet::Link, model::Subnet::Link> addDemux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::Link &x) {
  return {
      builder.addCell(model::AND, ~s, x),
      builder.addCell(model::AND,  s, x)
  };
}

// Multibit DEMUX 1-to-2 (S, X[*]):
// OUT[i] = ((i / |X|) == INDEX(S)) ? X[i % |X|] : 0.
model::Subnet::LinkList addDemux2(
    model::SubnetBuilder &builder,
    const model::Subnet::Link &s,
    const model::Subnet::LinkList &x) {
  model::Subnet::LinkList links(x.size() << 1);
  for (size_t i = 0; i < x.size(); ++i) {
    std::tie(links[i], links[x.size() + 1]) = addDemux2(builder, s, x[i]);
  }
  return links;
}

// Bitwise MUX 2-to-1 (S[*], X[*], Y[*]), |S| == |X| == |Y|:
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

// Multibit MUX *-to-1 (S[*], X[*]), |X| == |OUT| * 2^|S|:
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

// Bitwise DEMUX 1-to-2 (S[*], X[*]):
// OUT[s][i] = (S[i] == s) ? X[i] : 0, s=0,1.
model::SubnetID synthBDemux(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;
  const auto s = builder.addInputs(attr.getInWidth(0));
  const auto x = builder.addInputs(attr.getInWidth(1));
  assert(s.size() == x.size());

  model::Subnet::LinkList out0(x.size());
  model::Subnet::LinkList out1(x.size());
  for (size_t i = 0; i < s.size(); ++i) {
    std::tie(out0[i], out1[i]) = addDemux2(builder, s[i], x[i]);
  }

  builder.addOutputs(out0);
  builder.addOutputs(out1);
  return builder.make();
}

// Multibit DEMUX 1-to-2 (S, X[*]):
// OUT[s][i] = (S == s) ? X[i] : 0, s=0,1.
model::SubnetID synthDemux2(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;
  const auto s = builder.addInput();
  const auto x = builder.addInputs(attr.getInWidth(1));

  model::Subnet::LinkList out(x.size() << 1);
  for (size_t i = 0; i < x.size(); ++i) {
    std::tie(out[i], out[x.size() + i]) = addDemux2(builder, s, x[i]);
  }

  builder.addOutputs(out);
  return builder.make();
}

// Multibit DEMUX 1-to-* (S[*], X[*]), |OUT| == |X| * 2^|S|:
// OUT[i] = ((i / |X|) == INDEX(S)) ? X[i % |X|] : 0.
model::SubnetID synthDemux(const model::CellTypeAttr &attr) {
  model::SubnetBuilder builder;
  const auto s = builder.addInputs(attr.getInWidth(0));

  auto x = builder.addInputs(attr.getInWidth(1));
  for (size_t i = 0; i < s.size(); ++i) {
    x = addDemux2(builder, s[i], x);
  }

  builder.addOutputs(x);
  return builder.make();
}

} // namespace eda::gate::synthesizer
