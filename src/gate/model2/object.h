//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstddef>
#include <cstdint>

namespace eda::gate::model {

/// Full object identifier: [ tag:8 | short identifier | alignment zeros ].
template <uint64_t TAG, size_t SIZE, size_t LOG2>
class ObjectID final {
public:
  static constexpr uint64_t Tag = TAG;

  static constexpr size_t Size = SIZE;
  static constexpr size_t Log2 = LOG2;
  static_assert(Size == (1 << Log2));

  /// Makes the full identifier (FID) from the short one (SID).
  static ObjectID makeFID(uint64_t objectSID) {
    return (Tag << (64 - 8)) | (objectSID << Log2);
  }

  /// Makes the short identifier (SID) from full one (FID).
  static uint64_t makeSID(ObjectID objectFID) {
    return (objectFID & ~(0xfull << (64 - 8))) >> Log2;
  }

  ObjectID(uint64_t value): value(value) {}
  operator uint64_t() const { return value; }

  /// Returns the short identifier.
  uint64_t getSID() const { return makeSID(value); }

private:
  uint64_t value;
};

/// Null object identifier.
static constexpr uint64_t OBJ_NULL_ID = 0;

/// Object tag.
enum ObjectTag : uint8_t {
  TAG_NULL,
  TAG_CELL,
  TAG_CELL_TYPE,
  TAG_CELL_TYPE_ATTR,
  TAG_LINK,
  TAG_NET,
  TAG_STRING,
  TAG_LIST
};

using CellID         = ObjectID<TAG_CELL, 32, 5>;
using CellTypeID     = ObjectID<TAG_CELL_TYPE, 32, 5>;
using CellTypeAttrID = ObjectID<TAG_CELL_TYPE_ATTR, 1024, 10>;
using LinkID         = ObjectID<TAG_LINK, 8, 3>;
using NetID          = ObjectID<TAG_NET, 64, 6>;
using StringID       = ObjectID<TAG_STRING, 32, 5>;
using ListID         = ObjectID<TAG_LIST, 32, 5>;

} // namespace eda::gate::model
