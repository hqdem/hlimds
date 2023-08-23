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

/// Null object identifier.
static constexpr uint64_t OBJ_NULL_ID = 0;

/// Full object identifier (FID):
/// [ tag:8 | short object identifier (SID) | alignment zeros ].
template <uint64_t TAG, size_t SIZE, size_t LOG2>
class ObjectID final {
public:
  static constexpr uint64_t Tag = TAG;

  static constexpr size_t Size = SIZE;
  static constexpr size_t Log2 = LOG2;
  static_assert(Size == (1 << Log2));

  /// Sets the tag to the untagged FID.
  static constexpr ObjectID makeTaggedFID(uint64_t objectFID) {
    return (Tag << (64 - 8)) | objectFID;
  }

  /// Resets the tag of the tagged FID.
  static constexpr ObjectID makeUntaggedFID(uint64_t objectFID) {
    return objectFID & ~(0xfull << (64 - 8));
  }

  /// Makes the FID from the SID.
  static constexpr ObjectID makeFID(uint64_t objectSID) {
    return makeTaggedFID(objectSID << Log2);
  }

  /// Makes the SID from the FID.
  static constexpr uint64_t makeSID(ObjectID objectFID) {
    return makeUntaggedFID(objectFID) >> Log2;
  }

  ObjectID(uint64_t value): value(value) {}
  ObjectID(): ObjectID(OBJ_NULL_ID) {}
  operator uint64_t() const { return value; }

  /// Returns the SID.
  uint64_t getSID() const { return makeSID(value); }
  /// Returns the untagged FID.
  uint64_t getUntaggedFID() const { return makeUntaggedFID(value); }

private:
  const uint64_t value;
};

/// Object tag.
enum ObjectTag : uint8_t {
  TAG_NULL,
  TAG_CELL,
  TAG_CELL_TYPE,
  TAG_CELL_TYPE_ATTR,
  TAG_LINK,
  TAG_NET,
  TAG_STRING,
  TAG_LIST_BLOCK
};

using CellID         = ObjectID<TAG_CELL, 32, 5>;
using CellTypeID     = ObjectID<TAG_CELL_TYPE, 32, 5>;
using CellTypeAttrID = ObjectID<TAG_CELL_TYPE_ATTR, 1024, 10>;
using LinkID         = ObjectID<TAG_LINK, 8, 3>;
using NetID          = ObjectID<TAG_NET, 64, 6>;
using StringID       = ObjectID<TAG_STRING, 32, 5>;
using ListBlockID    = ObjectID<TAG_LIST_BLOCK, 64, 6>;

using ListID = ListBlockID;

} // namespace eda::gate::model
