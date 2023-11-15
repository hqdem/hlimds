//===----------------------------------------------------------------------===//
//
// part of the utopia eda project, under the apache license v2.0
// spdx-license-identifier: apache-2.0
// copyright 2021 isp ras (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"

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

  /// Returns the identifier of the source cell.
  CellID getCellID() const { return CellID::makeFID(value >> 24); }
  /// Returns the reference to the source cell.
  const Cell &getCell() const;
  /// Returns the output port of the source cell.
  uint16_t getPort() const { return (value >> 8) & 0xffff; }
  /// Checks whether the link is valid.
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
  Link(const LinkEnd &source, const LinkEnd &target):
      source(source), target(target) {}

  LinkEnd source;
  LinkEnd target;
};

static_assert(sizeof(Link) == LinkID::Size);

} // namespace eda::gate::model
