//===----------------------------------------------------------------------===//
//
// part of the utopia eda project, under the apache license v2.0
// spdx-license-identifier: apache-2.0
// copyright 2021 isp ras (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"

#include <functional>

namespace eda::gate::model {

class Cell;

//===----------------------------------------------------------------------===//
// Link End
//===----------------------------------------------------------------------===//

class LinkEnd final : public Object<LinkEnd, LinkEndID> {
public:
  static LinkEnd unpack(uint64_t value) {
    CellID cellID = CellID::makeFID(value >> 24);
    uint16_t port = (value >> 8) & 0xffff;
    bool valid = value & 1;

    return valid ? LinkEnd(cellID, port) : LinkEnd();
  }

  static uint64_t pack(LinkEnd link) {
    return link.value;
  }

  LinkEnd(): value(0) {}

  LinkEnd(CellID cellID, uint16_t port):
      value((cellID.getSID() << 24) | (static_cast<uint64_t>(port) << 8) | 1) {}

  explicit LinkEnd(CellID cellID): LinkEnd(cellID, 0) {}

  LinkEnd(const LinkEnd &) = default;
  LinkEnd &operator =(const LinkEnd &) = default;
  bool operator ==(const LinkEnd &r) const noexcept { return value == r.value; }

  /// Returns the identifier of the source cell.
  CellID getCellID() const { return CellID::makeFID(value >> 24); }
  /// Returns the reference to the source cell.
  const Cell &getCell() const;
  /// Returns the output port of the source cell.
  uint16_t getPort() const { return (value >> 8) & 0xffff; }
  /// Checks whether the link-end is valid.
  bool isValid() const { return value & 1; }

private:
  /// Packed value: [ cell SID:40 | port:16 | valid:1]
  uint64_t value;
};

static_assert(sizeof(LinkEnd) == LinkEndID::Size);

//===----------------------------------------------------------------------===//
// Link
//===----------------------------------------------------------------------===//

struct Link final : public Object<Link, LinkID> {
  Link() {}
  Link(const LinkEnd &source, const LinkEnd &target):
      source(source), target(target) {}
  Link(CellID sourceID, uint16_t sourcePort,
       CellID targetID, uint16_t targetPort):
      Link(LinkEnd{sourceID, sourcePort}, LinkEnd{targetID, targetPort}) {}

  Link(const Link &) = default;
  Link &operator =(const Link &) = default;

  bool operator ==(const Link &r) const noexcept {
    return source == r.source && target == r.target;
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


