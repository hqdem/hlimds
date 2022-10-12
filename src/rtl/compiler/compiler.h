//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

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
class Compiler final {
public:
  Compiler(FLibrary &library): _library(library) {
    _gateIds.reserve(1024*1024);
  }

  /// Compiles the gate-level net from the RTL net.
  std::unique_ptr<GNet> compile(const Net &net);

private:
  FLibrary &_library;

  Gate::Id gateId(VNode::Id vnodeId) const;

  void allocGates(const VNode *vnode, GNet &net);

  void synthSrc(const VNode *vnode, GNet &net);
  void synthVal(const VNode *vnode, GNet &net);
  void synthFun(const VNode *vnode, GNet &net);
  void synthMux(const VNode *vnode, GNet &net);
  void synthReg(const VNode *vnode, GNet &net);

  GNet::In in(const VNode *vnode);
  GNet::Out out(const VNode *vnode);

  // Maps vnodes to the identifiers of their lower bits' gates.
  std::unordered_map<VNode::Id, Gate::Id> _gateIds;
};

} // namespace eda::rtl::compiler
