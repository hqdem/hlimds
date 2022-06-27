//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/gnet.h"
#include "rtl/model/fsymbol.h"

#include <cassert>
#include <memory>
#include <vector>

using namespace eda::gate::model;
using namespace eda::rtl::model;

namespace eda::gate::model {
  class GNet;
} // namespace eda::gate::model

namespace eda::rtl::library {

/**
 * \brief Interface for functional library.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>.
 */
struct FLibrary {
  using GateIdList = GNet::GateIdList;
  using Value = GNet::Value;
  using In = GNet::In;
  using Out = GNet::Out;

  /// Checks if the library supports the given function.
  virtual bool supports(FuncSymbol func) const = 0;

  /// Synthesize the net for the given value.
  virtual void synth(const Out &out,
                     const Value &value,
                     GNet &net) = 0;

  /// Synthesize the net for the given function.
  virtual void synth(FuncSymbol func,
                     const Out &out,
                     const In &in,
                     GNet &net) = 0;

  /// Synthesize the net for the given register.
  virtual void synth(const Out &out,
                     const In &in,
                     const Signal::List &control,
                     GNet &net) = 0;

  virtual ~FLibrary() {} 
};

/**
 * \brief Functional library default implementation.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class FLibraryDefault final: public FLibrary {
public:
  static FLibrary& get() {
    static auto instance = std::unique_ptr<FLibrary>(new FLibraryDefault());
    return *instance;
  }

  bool supports(FuncSymbol func) const override;

  void synth(const Out &out,
             const Value &value,
             GNet &net) override;

  void synth(FuncSymbol func,
             const Out &out,
             const In &in,
             GNet &net) override;

  void synth(const Out &out,
             const In &in,
             const Signal::List &control,
             GNet &net) override;

private:
  FLibraryDefault() {}
  ~FLibraryDefault() override {}

  static void synthAdd(const Out &out, const In &in, GNet &net);
  static void synthSub(const Out &out, const In &in, GNet &net);
  static void synthMux(const Out &out, const In &in, GNet &net);

  static void synthAdder(const Out &out, const In &in, bool plusOne, GNet &net);

  static void synthAdder(Gate::Id z,
                         Gate::Id carryOut,
                         Gate::Id x,
                         Gate::Id y,
                         Gate::Id carryIn,
                         GNet &net);

  static Signal invertIfNegative(const Signal &event, GNet &net);

  template<GateSymbol G>
  static void synthUnaryBitwiseOp(const Out &out, const In &in, GNet &net);

  template<GateSymbol G>
  static void synthBinaryBitwiseOp(const Out &out, const In &in, GNet &net);
};

template<GateSymbol G>
void FLibraryDefault::synthUnaryBitwiseOp(const Out &out,
                                          const In &in,
                                          GNet &net) {
  assert(in.size() == 1);

  const auto &x = in[0];
  assert(out.size() == x.size());

  for (std::size_t i = 0; i < out.size(); i++) {
    auto xi = Signal::always(x[i]);
    net.setGate(out[i], G, { xi });
  }
}

template<GateSymbol G>
void FLibraryDefault::synthBinaryBitwiseOp(const Out &out,
                                           const In &in,
                                           GNet &net) {
  assert(in.size() == 2);

  const auto &x = in[0];
  const auto &y = in[1];
  assert(x.size() == y.size() && out.size() == x.size());

  for (std::size_t i = 0; i < out.size(); i++) {
    auto xi = Signal::always(x[i]);
    auto yi = Signal::always(y[i]);
    net.setGate(out[i], G, { xi, yi });
  }
}

} // namespace eda::rtl::library
