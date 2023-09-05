//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/memory.h"
#include "util/singleton.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Object Identifier
//===----------------------------------------------------------------------===//

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

using CellID         = ObjectID<TAG_CELL, 32, 5>;
using CellTypeID     = ObjectID<TAG_CELL_TYPE, 32, 5>;
using CellTypeAttrID = ObjectID<TAG_CELL_TYPE_ATTR, 1024, 10>;
using LinkEndID      = ObjectID<TAG_LINK_END, 8, 3>;
using LinkID         = ObjectID<TAG_LINK, 16, 4>;
using NetID          = ObjectID<TAG_NET, 64, 6>;
using SubnetID       = ObjectID<TAG_SUBNET, 16, 4>;
using StringID       = ObjectID<TAG_STRING, 32, 5>;
using ListBlockID    = ObjectID<TAG_LIST_BLOCK, 64, 6>;

using ListID = ListBlockID;

//===----------------------------------------------------------------------===//
// Object Identifier
//===----------------------------------------------------------------------===//

template<typename T>
class Storage : public util::Singleton<Storage<T>> {
public:
  /// TODO: Dummy (to be implemented).
  template<typename... Args>
  typename T::ID allocateExt(size_t size, Args&&... args) {
    assert(size >= T::ID::Size && size <= PAGE_SIZE);

    // If there is no place in the current page, allocate a new one.
    if (systemPage == nullptr || (offset + size) < PAGE_SIZE) {
      const auto translation = PageManager::get().allocate();

      objectPage = translation.first;
      systemPage = translation.second;

      offset = 0;
    }

    new(PageManager::getObjectPtr(systemPage, offset)) T(args...);
    auto untaggedID = PageManager::getObjectID(objectPage, offset);

    offset += size;

    return T::ID::makeTaggedFID(untaggedID);
  }

  template<typename... Args>
  typename T::ID allocate(Args&&... args) {
    return allocateExt(T::ID::Size, args...);
  }

  /// TODO: Dummy (to be implemented).
  T *access(typename T::ID objectFID) {
    if (objectFID == OBJ_NULL_ID) {
      return nullptr;
    }

    const typename T::ID untaggedFID = T::ID::makeUntaggedFID(objectFID);

    const auto objectPage = PageManager::getPage(untaggedFID);
    const auto offset = PageManager::getOffset(untaggedFID);

    const SystemPage systemPage = PageManager::get().translate(objectPage);
    return reinterpret_cast<T*>(PageManager::getObjectPtr(systemPage, offset));
  }

  /// TODO: Dummy (to be implemented).
  void release(typename T::ID objectID) {
    // Do nothing.
  }

private:
  /// Current object page.
  ObjectPage objectPage = -1;
  /// Current system page.
  SystemPage systemPage = nullptr;
  /// Current offset.
  size_t offset = 0;
};

template<typename T, typename... Args>
typename T::ID allocateExt(size_t size, Args&&... args) {
  return Storage<T>::get().allocateExt(size, args...);
}

template<typename T, typename... Args>
typename T::ID allocate(Args&&... args) {
  return Storage<T>::get().allocate(args...);
}

template<typename T>
T *access(typename T::ID objectID) {
  return Storage<T>::get().access(objectID);
}

template<typename T>
void release(typename T::ID objectID) {
  Storage<T>::get().release(objectID);
}

//===----------------------------------------------------------------------===//
// Object Template
//===----------------------------------------------------------------------===//

template <typename T, typename TID>
struct Object {
  using ID = TID;

  static T &get(TID objectID) { return *access<T>(objectID); }

  static constexpr TID makeFID(uint64_t objectSID) {
    return TID::makeFID(objectSID);
  }

  static constexpr uint64_t makeSID(TID objectFID) {
    return TID::makeSID(objectFID);
  }
};

} // namespace eda::gate::model
