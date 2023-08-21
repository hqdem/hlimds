//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"

#include <vector>

namespace eda::gate::model {

template<typename T>
class Storage {
public:
  /// TODO: Dummy (to be implemented).
  template<typename... Args>
  static typename T::ID allocate(Args&&... args) {
    T object(args...);
    objects.push_back(object);
    return static_cast<typename T::ID>(reinterpret_cast<uint64_t>(&objects.back()));
  }

  /// TODO: Dummy (to be implemented).
  static T *access(typename T::ID objectID) {
    return reinterpret_cast<T*>(static_cast<uint64_t>(objectID));
  }

  /// TODO: Dummy (to be implemented).
  static void release(typename T::ID objectID) {
    // Do nothing.
  }

private:
  static std::vector<T> objects;
};

template<typename T>
std::vector<T> Storage<T>::objects;

template<typename T, typename... Args>
typename T::ID allocate(Args&&... args) {
  return Storage<T>::allocate(args...);
}

template<typename T>
T *access(typename T::ID objectID) {
  return Storage<T>::access(objectID);
}

template<typename T>
void release(typename T::ID objectID) {
  Storage<T>::release(objectID);
}

} // namespace eda::gate::model
