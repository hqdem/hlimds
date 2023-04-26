//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "base/engine/engine.h"
#include "gate/model/gnet.h"
#include "rtl/library/flibrary.h"
#include "rtl/model/net.h"
#include "rtl/model/vnode.h"

#include <cassert>
#include <memory>
#include <unordered_map>

using namespace eda::gate::model;
using namespace eda::rtl::library;
using namespace eda::rtl::model;

namespace eda::rtl::compiler {

/**
 * \brief Implements a gate-level net compiler (logic synthesizer).
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Compiler final : public eda::base::engine::Engine {
  using Context = eda::base::engine::Context;
  using Engine = eda::base::engine::Engine;
  using Status = eda::base::engine::Status;
  using VNodeIdMap = std::unordered_map<VNode::Id, GNet::Out>;

public:
  static constexpr auto ID = "rtl.compiler";

  Compiler(FLibrary &library):
      Engine(ID), _library(library) {}

  /// Executes the engine.
  Status execute(const Context &context) const override {
    const auto &net = context.store.get<Net>("vnet");

    auto gnet = compile(net);
    context.store.add<std::unique_ptr<GNet>>("gnet0", std::move(gnet));

    return Status::SUCCESS;
  }

  /// Compiles the gate-level net from the RTL net.
  std::unique_ptr<GNet> compile(const Net &net) const;

private:
  void synthSrc(const VNode *vnode, GNet &net, VNodeIdMap &map) const;
  void synthVal(const VNode *vnode, GNet &net, VNodeIdMap &map) const;
  void synthOut(const VNode *vnode, GNet &net, VNodeIdMap &map) const;
  void synthFun(const VNode *vnode, GNet &net, VNodeIdMap &map) const;
  void synthMux(const VNode *vnode, GNet &net, VNodeIdMap &map) const;
  void allocReg(const VNode *vnode, GNet &net, VNodeIdMap &map) const;
  void synthReg(const VNode *vnode, GNet &net, VNodeIdMap &map) const;

  GNet::In in(const VNode *vnode, const VNodeIdMap &map,
              size_t beginIndex, size_t endIndex) const;
  GNet::In in(const VNode *vnode, const VNodeIdMap &map) const;

  const GNet::Out &out(const VNode *vnode, const VNodeIdMap &map) const;
  const GNet::Out &out(VNode::Id vnodeId, const VNodeIdMap &map) const;

  const FLibrary &_library;
};

} // namespace eda::rtl::compiler
