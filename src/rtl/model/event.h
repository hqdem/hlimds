//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <iostream>
#include <vector>

namespace eda::rtl::model {

class VNode;

/**
 * \brief Represents a triggering event.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Event final {
public:
  using List = std::vector<Event>;

  enum Kind {
    /// Positive edge: always_ff @(posedge <node>) begin <action> end.
    POSEDGE,
    /// Negative edge: always_ff @(negedge <node>) begin <action> end.
    NEGEDGE,
    /// Low level: always_latch begin if (~<node>) <action> end.
    LEVEL0,
    /// High Level: always_latch begin if (<node>) <action> end.
    LEVEL1,
    /// Continuous: always_comb begin <action> end.
    ALWAYS,
    /// Explicit delay: #<delay> <action>.
    DELAY
  };

  Event(Kind kind, const VNode *node = nullptr):
    _kind(kind), _node(node), _delay(0) {}

  Event(std::size_t delay):
    _kind(DELAY), _node(nullptr), _delay(delay) {}

  Event():
    _kind(ALWAYS), _node(nullptr), _delay(0) {}

  bool edge() const { return _kind == POSEDGE || _kind == NEGEDGE; }
  bool level() const { return _kind == LEVEL0 || _kind == LEVEL1; }

  Kind kind() const { return _kind; }
  const VNode* node() const { return _node; }
  std::size_t delay() const { return _delay; }

  bool operator ==(const Event &rhs) const {
    if (&rhs == this) {
      return true;
    }

    return _kind == rhs._kind && _node == rhs._node && _delay == rhs._delay;
  }

private:
  // Event kind.
  const Kind _kind;
  // Single-bit node for tracking events on (for edges and levels only).
  const VNode *_node;
  // Delay value.
  const std::size_t _delay;
};

std::ostream& operator <<(std::ostream &out, const Event::Kind &kind);
std::ostream& operator <<(std::ostream &out, const Event &event);

} // namespace eda::rtl::model
