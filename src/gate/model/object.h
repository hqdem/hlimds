//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/memory.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Object Identifier
//===----------------------------------------------------------------------===//

/// Untyped null object identifier.
static constexpr uint64_t OBJ_NULL_ID = 0;

/// Full object identifier (FID):
/// | tag:8 | short object identifier (SID) | zeros:Z |
/// | 63 56 | 55 (V+Z-1)                  Z | Z-1   0 |.
template <uint64_t T, size_t S, size_t V, size_t Z>
class ObjectID final {
public:
  /// Object tag.
  static constexpr uint64_t Tag = T;
  static_assert(0x01 <= T && T <= 0xff);

  /// Object size in bytes.
  static constexpr size_t Size = S;
  static_assert(S <= LARGE_PAGE_SIZE);

  /// Number of alignment zeros: log2(size).
  static constexpr size_t Log2 = Z;
  static_assert(S == (1u << Z));

  /// Number of valuable bits: SID width.
  static constexpr size_t Bits = V;
  static_assert((V + Z) <= (64 - 8));

  /// Invalid (null) FID.
  static constexpr uint64_t NullFID = OBJ_NULL_ID;
  /// Invalid (null) SID.
  static constexpr uint64_t NullSID = (1ull << V) - 1ull;

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
    return objectSID == NullSID ? ObjectID(NullFID)
                                : makeTaggedFID(objectSID << Log2);
  }

  /// Makes the SID from the FID.
  static constexpr uint64_t makeSID(ObjectID objectFID) {
    return objectFID == NullFID ? NullSID
                                : makeUntaggedFID(objectFID) >> Log2;
  }

  /// Returns the tag of the FID.
  static constexpr uint64_t getTag(uint64_t objectFID) {
    return (objectFID >> (64 - 8)) & 0xff;
  }

  /// Checks that the FID is of the required type.
  static constexpr bool checkTag(uint64_t objectFID) {
    return getTag(objectFID) == Tag;
  }

  /// Constructs a FID from the specified value.
  ObjectID(uint64_t value): value(value) {}
  /// Constructs a null-valued FID.
  ObjectID(): ObjectID(OBJ_NULL_ID) {}
  /// Constructs a copy of the FID.
  ObjectID(const ObjectID &r): value(r.value) {}

  /// Converts the FID to the 64-bit unsigned integer.
  operator uint64_t() const { return value; }

  /// Assigns the value to this FID.
  ObjectID &operator =(ObjectID r) {
    value = r.value;
    return *this;
  }

  /// Returns the SID.
  uint64_t getSID() const { return makeSID(value); }
  /// Returns the FID.
  uint64_t getFID() const { return value; }
  /// Returns the untagged FID.
  uint64_t getUntaggedFID() const { return makeUntaggedFID(value); }

private:
  /// Tagged FID.
  uint64_t value;
};

/// Object tag.
enum ObjectTag : uint8_t {
  TAG_NULL,
  TAG_CELL,
  TAG_CELL_TYPE,
  TAG_CELL_TYPE_ATTR,
  TAG_LINK_END,
  TAG_LINK,
  TAG_NET,
  TAG_SUBNET,
  TAG_STRING,
  TAG_LIST_BLOCK
};

//                     ObjectID<Tag, Bytes, |SID|, Zeros>
using CellID         = ObjectID<TAG_CELL, 16, 40, 4>;
using CellTypeID     = ObjectID<TAG_CELL_TYPE, 32, 32, 5>;
using CellTypeAttrID = ObjectID<TAG_CELL_TYPE_ATTR, 1024, 32, 10>;
using LinkEndID      = ObjectID<TAG_LINK_END, 8, 50, 3>;
using LinkID         = ObjectID<TAG_LINK, 16, 50, 4>;
using NetID          = ObjectID<TAG_NET, 64, 32, 6>;
using SubnetID       = ObjectID<TAG_SUBNET, 32, 32, 5>;
using StringID       = ObjectID<TAG_STRING, 32, 32, 5>;
using ListBlockID    = ObjectID<TAG_LIST_BLOCK, 64, 32, 6>;

using ListID = ListBlockID;

//===----------------------------------------------------------------------===//
// Object Template
//===----------------------------------------------------------------------===//

template <typename T, typename TID>
struct Object {
  using ID = TID;

  // Defined in storage.h.
  static T &get(TID objectID);
  static void release(TID objectID);

  static constexpr TID makeFID(uint64_t objectSID) {
    return TID::makeFID(objectSID);
  }

  static constexpr uint64_t makeSID(TID objectFID) {
    return TID::makeSID(objectFID);
  }
};

} // namespace eda::gate::model

template <uint64_t T, size_t S, size_t V, size_t Z>
struct std::hash<eda::gate::model::ObjectID<T, S, V, Z>> {
  size_t operator() (const eda::gate::model::ObjectID<T, S, V, Z> &o) const {
    return static_cast<uint64_t>(o);
  }
};
