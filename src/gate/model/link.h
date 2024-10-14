//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/object.h"
#include "gate/model/storage.h"

#include <functional>

namespace eda::gate::model {

class Cell;

//===----------------------------------------------------------------------===//
// Link End
//===----------------------------------------------------------------------===//

class LinkEnd final : public Object<LinkEnd, LinkEndID> {
public:
  using PortType = uint32_t;

  static constexpr auto PortBits = 23;
  static constexpr auto PortMask = (1u << PortBits) - 1;

  static LinkEnd unpack(const uint64_t value) {
    CellID cellID = CellID::makeFID(value >> (PortBits + 1));
    uint32_t port = (value >> 1) & PortMask;
    bool valid = value & 1;

    return valid ? LinkEnd(cellID, port) : LinkEnd();
  }

  static uint64_t pack(const LinkEnd link) {
    return link.value;
  }

  LinkEnd(): value(0) {}

  LinkEnd(const CellID cellID, const PortType port):
      value((cellID.getSID() << (PortBits + 1))
          | (static_cast<uint64_t>(port) << 1) | 1) {}

  explicit LinkEnd(const CellID cellID): LinkEnd(cellID, 0) {}

  LinkEnd(const LinkEnd &) = default;
  LinkEnd &operator=(const LinkEnd &) = default;
  bool operator==(const LinkEnd &other) const noexcept {
    return value == other.value;
  }

  /// Returns the identifier of the source cell.
  CellID getCellID() const { return CellID::makeFID(value >> (PortBits + 1)); }
  /// Returns the reference to the source cell.
  const Cell &getCell() const;
  /// Returns the output port of the source cell.
  PortType getPort() const { return (value >> 1) & PortMask; }
  /// Checks whether the link-end is valid.
  bool isValid() const { return value & 1; }

private:
  /// Packed value: [cell SID:40 | port:23 | valid:1].
  uint64_t value;
};

static_assert(sizeof(LinkEnd) == LinkEndID::Size);

//===----------------------------------------------------------------------===//
// Link
//===----------------------------------------------------------------------===//

struct Link final : public Object<Link, LinkID> {
  using PortType = LinkEnd::PortType;

  Link() {}
  Link(const LinkEnd &source, const LinkEnd &target):
      source(source), target(target) {}
  Link(const CellID sourceID, const PortType sourcePort,
       const CellID targetID, const PortType targetPort):
      Link(LinkEnd{sourceID, sourcePort}, LinkEnd{targetID, targetPort}) {}

  Link(const Link &) = default;
  Link &operator=(const Link &) = default;

  bool operator==(const Link &other) const noexcept {
    return source == other.source && target == other.target;
  }

  LinkEnd source;
  LinkEnd target;
};

static_assert(sizeof(Link) == LinkID::Size);

} // namespace eda::gate::model

template<>
struct std::hash<eda::gate::model::Link> {
  using Link = eda::gate::model::Link;
  using LinkEnd = eda::gate::model::LinkEnd;

  size_t operator()(const Link &link) const noexcept {
    const auto h1 = std::hash<uint64_t>{}(LinkEnd::pack(link.source));
    const auto h2 = std::hash<uint64_t>{}(LinkEnd::pack(link.target));
    return h1 ^ (h2 << 1);
  }
};
