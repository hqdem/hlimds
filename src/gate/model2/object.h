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
/// | tag:8 | short object identifier (SID) | zeros:Z |
/// | 63 56 | 55                          Z | Z-1   0 |.
template <uint64_t T, size_t S, size_t Z>
class ObjectID final {
public:
  static constexpr uint64_t Tag = T;

  static constexpr size_t Size = S;
  static constexpr size_t Log2 = Z;
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

  /// Constructs a FID from the specified value.
  ObjectID(uint64_t value): value(value) {}
  /// Constructs a null-valued FID.
  ObjectID(): ObjectID(OBJ_NULL_ID) {}

  /// Converts the FID to the 64-bit unsigned integer.
  operator uint64_t() const { return value; }

  /// Assigns the value to this FID.
  ObjectID &operator =(ObjectID r) {
    value = r.value;
    return *this;
  }

  /// Returns the SID.
  uint64_t getSID() const { return makeSID(value); }
  /// Returns the untagged FID.
  uint64_t getUntaggedFID() const { return makeUntaggedFID(value); }

private:
  /// FID value.
  uint64_t value;
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
