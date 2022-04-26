//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cassert>
#include <memory>
#include <unordered_map>

#include "gate/model/netlist.h"
#include "rtl/library/flibrary.h"
#include "rtl/model/net.h"
#include "rtl/model/vnode.h"

using namespace eda::gate::model;
using namespace eda::rtl::library;
using namespace eda::rtl::model;

namespace eda::rtl::compiler {

/**
 * \brief Implements a gate-level netlist compiler (logic synthesizer).
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Compiler final {
public:
  Compiler(FLibrary &library): _library(library) {
    _gates_id.reserve(1024*1024);
  }

  /// Compiles the gate-level netlist from the RTL net.
  std::unique_ptr<Netlist> compile(const Net &net);

private:
  FLibrary &_library;

  Gate::Id gate_id(const VNode *vnode) const;
  Gate::Id gate_id(const VNode *vnode, const Netlist &netlist);

  void alloc_gates(const VNode *vnode, Netlist &netlist);

  void synth_src(const VNode *vnode, Netlist &netlist);
  void synth_val(const VNode *vnode, Netlist &netlist);
  void synth_fun(const VNode *vnode, Netlist &netlist);
  void synth_mux(const VNode *vnode, Netlist &netlist);
  void synth_reg(const VNode *vnode, Netlist &netlist);

  Netlist::In in(const VNode *vnode);
  Netlist::Out out(const VNode *vnode);

  // Maps vnodes to the identifiers of their lower bits' gates.
  std::unordered_map<std::string, Gate::Id> _gates_id;
};

} // namespace eda::rtl::compiler
