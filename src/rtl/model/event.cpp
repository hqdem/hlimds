//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "rtl/model/event.h"
#include "rtl/model/vnode.h"

namespace eda::rtl::model {

std::ostream &operator <<(std::ostream &out, const Event::Kind &kind) {
  switch (kind) {
  case Event::POSEDGE:
    return out << "posedge";
  case Event::NEGEDGE:
    return out << "negedge";
  case Event::LEVEL0:
    return out << "level0";
  case Event::LEVEL1:
    return out << "level1";
  case Event::ALWAYS:
    return out << "*";
  case Event::DELAY:
    return out << "#";
  }
  return out;
}

std::ostream &operator <<(std::ostream &out, const Event &event) {
  if (event.kind() == Event::ALWAYS) {
    return out << "*";
  }
  if (event.kind() == Event::DELAY) {
    return out << "#" << event.delay();
  }
  return out << event.kind() << "(" << event.node()->name() << ")";
}

} // namespace eda::rtl::model
