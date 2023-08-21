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

#pragma pack(push, 1)
class Link final {
public:
  using ID = LinkID;

  Link():
      cellSID(0), port(0), valid(0) {}

  Link(CellID cellID, uint16_t port):
      cellSID(cellID.getSID()), port(port), valid(1) {}

  Link(const Link &) = default;
  Link &operator =(const Link &) = default;

  CellID getSourceID() const { return CellID::makeFID(cellSID); }

  uint16_t getSourcePort() const { return port; }

  bool isValid() const { return valid; }

private:
  /// Source cell short identifier.
  uint64_t cellSID : 40;
  /// Output port of the source cell.
  uint64_t port : 16;

  /// Properties.
  uint64_t valid : 1;
};
#pragma pack(pop)

static_assert(sizeof(Link) == LinkID::Size);

} // namespace eda::gate::model
