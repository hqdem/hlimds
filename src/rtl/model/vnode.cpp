//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "rtl/model/vnode.h"

namespace eda::rtl::model {

static std::ostream& operator <<(std::ostream &out, const std::vector<bool> &value) {
  for (bool bit: value) {
    out << bit;
  }

  return out;
}

static std::ostream& operator <<(std::ostream &out, const VNode::List &vnodes) {
  bool separator = false;
  for (VNode *vnode: vnodes) {
    out << (separator ? ", " : "") << vnode->name();
    separator = true;
  }

  return out;
}

std::ostream& operator <<(std::ostream &out, const VNode &vnode) {
  switch (vnode.kind()) {
  case VNode::SRC:
    return out << "S{" << vnode.var() << "}";
  case VNode::VAL:
    return out << "C{" << vnode.var() << " = " << vnode.value()<< "}";
  case VNode::FUN:
    return out << "F{" << vnode.var() << " = " << vnode.func() << "(" << vnode.inputs() << ")}";
  case VNode::MUX:
    return out << "M{" << vnode.var() << " = mux(" << vnode.inputs() << ")}";
  case VNode::REG:
    out << "R{";
    bool separator = false;
    for (std::size_t i = 0; i < vnode.arity(); i++) {
      out << (separator ? ", " : "");
      if (i < vnode.esize()) {
        out << vnode.event(i) << ": ";
      }
      out << vnode.var() << " = " << vnode.input(i)->name();
      separator = true;
    }
    return out << "}";
  }

  return out;
}

} // namespace eda::rtl::model
