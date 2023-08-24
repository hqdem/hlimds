//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/memory.h"
#include "gate/model2/object.h"
#include "util/singleton.h"

#include <cassert>

namespace eda::gate::model {

template<typename T>
class Storage : public util::Singleton<Storage<T>> {
public:
  /// TODO: Dummy (to be implemented).
  template<typename... Args>
  typename T::ID allocateExt(size_t size, Args&&... args) {
    assert(size >= T::ID::Size && size <= PAGE_SIZE);

    // If there is no place in the current page, a new one is allocated.
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

} // namespace eda::gate::model
