//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/memory.h"
#include "gate/model/object.h"
#include "util/singleton.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace eda::gate::model {

/**
 * @brief Describes object allocation in memory.
 */
template <typename T /* Object type */>
struct ObjDesc final {
  uint64_t valid:1;
  uint64_t reserved:63;
  T *objPtr{nullptr};
  __uint128_t globalID;
};

static_assert(sizeof(ObjDesc<int /* Doesn't matter */>) == 32);

/**
 * @brief Table for T-object descriptors.
 */
template <typename T /* Object type */>
struct ObjDescTable final {
  static constexpr uint64_t ObjSize = T::ID::Size;
  static constexpr uint64_t ObjNum = 1ull << T::ID::Bits; 
  static constexpr uint64_t ObjDescSize = sizeof(ObjDesc<T>);
  static constexpr uint64_t ObjDescPageSize = SMALL_PAGE_SIZE;
  static constexpr uint64_t ObjDescPerPage = ObjDescPageSize / ObjDescSize;
  static constexpr uint64_t ObjDescPageNum = ObjNum / ObjDescPerPage;
  static constexpr uint64_t ObjDescIdxMask = ObjDescPerPage - 1;
  static_assert((ObjDescPerPage & (ObjDescPerPage - 1)) == 0);

  /**
   * @brief Returns the page-index pair for the given object ID.
   */
  static std::pair<size_t, size_t> getLocation(const typename T::ID objID) {
    const size_t objSID = objID.getSID();
    return {objSID / ObjDescPerPage, objSID & ObjDescIdxMask};
  }

  /**
   * @brief Returns the object descriptor presented in the table (no checks).
   */
  ObjDesc<T> *accessNoCheck(const typename T::ID objID) {
    const auto loc = getLocation(objID);
    return &table[loc.first][loc.second];
  }

  /**
   * @brief Returns the object descriptor (if it exists) or nullptr (otherwise).
   */
  ObjDesc<T> *accessCheck(const typename T::ID objID) {
    const auto loc = getLocation(objID);
    return table[loc.first] ? &table[loc.first][loc.second] : nullptr;
  }

  /**
   * @brief Returns the object descriptor (allocates the memory if required).
   */
  ObjDesc<T> *accessAlloc(const typename T::ID objID) {
    const auto loc = getLocation(objID);
    if (!table[loc.first]) {
      table[loc.first] = reinterpret_cast<ObjDesc<T>*>(
          PageManager::get().allocate(ObjDescPageSize));
    }
    return &table[loc.first][loc.second];
  }

private:
  ObjDesc<T> *table[ObjDescPageNum];
};

template<typename T>
class Storage : public util::Singleton<Storage<T>> {
public:
  static constexpr uint64_t ObjSize = T::ID::Size;
  static constexpr uint64_t ObjPageSize = LARGE_PAGE_SIZE;

  /**
   * @brief Allocates an object of the given size.
   */
  template<typename... Args>
  typename T::ID allocateExt(const size_t objSize, Args&&... args) {
    assert(objSize >= ObjSize && objSize <= ObjPageSize);

    // Align the address (offset is enough).
    offset = ((offset - 1) & ~(ObjSize - 1)) + ObjSize;

    // If there is no place in the current page, allocate a new one.
    if (page == nullptr || (offset + objSize) > ObjPageSize) {
      page = PageManager::get().allocate(ObjPageSize);
      offset = 0;
    }

    auto *location = PageManager::getObjPtr(page, offset);
    new(location) T(args...);
    offset += objSize;

    const auto objID = T::ID::makeFID(objSID++);
    desc.accessAlloc(objID)->objPtr = static_cast<T*>(location);

    return objID;
  }

  /**
   * @brief Allocates an object.
   */
  template<typename... Args>
  typename T::ID allocate(Args&&... args) {
    return allocateExt(ObjSize, args...);
  }

  /**
   * @brief Returns the pointer to the object.
   */
  T *access(const typename T::ID objID) {
    return objID != OBJ_NULL_ID ? desc.accessNoCheck(objID)->objPtr : nullptr;
  }

  /**
   * @brief Releases the object.
   */
  void release(typename T::ID objID) {
    // Do nothing.
  }

private:
  /// Current object SID.
  uint32_t objSID{0};
  /// Current system page.
  SystemPage page{nullptr};
  /// Current offset.
  size_t offset{0};

  /// Object descriptors.
  ObjDescTable<T> desc;
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
T *access(typename T::ID objID) {
  return Storage<T>::get().access(objID);
}

template<typename T>
void release(typename T::ID objID) {
  Storage<T>::get().release(objID);
}

template <typename T, typename TID>
T &Object<T, TID>::get(TID objID) { return *access<T>(objID); }

} // namespace eda::gate::model
