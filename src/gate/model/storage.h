//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

namespace eda::gate::model {

#pragma once

#include "gate/model/memory.h"
#include "gate/model/object.h"
#include "util/singleton.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

template<typename T>
class Storage : public util::Singleton<Storage<T>> {
public:
  /// TODO: Dummy (to be implemented).
  template<typename... Args>
  typename T::ID allocateExt(size_t size, Args&&... args) {
    constexpr auto SIZE = T::ID::Size;
    assert(size >= SIZE && size <= LARGE_PAGE_SIZE);

    // Align the address (offset is enough).
    offset = ((offset - 1) & ~(SIZE - 1)) + SIZE;

    // If there is no place in the current page, allocate a new one.
    if (systemPage == nullptr || (offset + size) > LARGE_PAGE_SIZE) {
      const auto translation = PageManager::get().allocate();

      objectPage = translation.first;
      systemPage = translation.second;

      offset = 0;
    }

    auto *location = PageManager::getObjectPtr(systemPage, offset);
    new(location) T(args...);

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
  ObjectPage objectPage{-1u};
  /// Current system page.
  SystemPage systemPage{nullptr};
  /// Current offset.
  size_t offset{0};
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

template <typename T, typename TID>
T &Object<T, TID>::get(TID objectID) { return *access<T>(objectID); }

} // namespace eda::gate::model
