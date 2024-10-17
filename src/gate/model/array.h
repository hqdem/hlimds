//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/list.h"

#include <algorithm>
#include <cassert>
#include <cstring>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Array Block
//===----------------------------------------------------------------------===//

template<typename T>
using ArrayBlock = ListBlock<T>;

using ArrayBlockID = ListBlockID;
using ArrayID = ListID;

//===----------------------------------------------------------------------===//
// Array Iterator
//===----------------------------------------------------------------------===//

template <typename T>
class Array;

template <typename T>
class ArrayIterator final {
  friend class Array<T>;

public:
  ArrayIterator(const ArrayIterator<T> &) = default;
  ArrayIterator &operator=(const ArrayIterator<T> &) = default;

  bool operator==(const ArrayIterator<T> &r) const {
    return block == r.block && index == r.index;
  }

  bool operator!=(const ArrayIterator<T> &r) const {
    return !(*this == r);
  }

  /// Prefix increment operator.
  ArrayIterator<T> &operator++() {
    ++index;
    return *this;
  }

  /// Postfix increment operator.
  ArrayIterator<T> operator++(int) {
    auto temp = *this;
    ++*this;
    return temp;
  }

  /// Dereferencing operator.
  T &operator*() {
    assert(block != nullptr);
    return reinterpret_cast<T&>(block->items[index]);
  }

private:
  /// Constructs the list iterator for the given block.
  ArrayIterator(ArrayBlock<T> *block, size_t index):
      block(block), index(index) {}

  ArrayBlock<T> *block;
  size_t index;
};

//===----------------------------------------------------------------------===//
// Array Interface
//===----------------------------------------------------------------------===//

template <typename T>
class Array final {
public:
  /// Constructs a wrapper around the given list structure.
  explicit Array(ArrayID arrayID):
      arrayID(arrayID), block(accessObject<ArrayBlock<T>>(arrayID)) {
    // Only single-block arrays are supported.
    assert(block != nullptr && block->begin && block->end);
  }

  /// Constructs a new array w/ the specified capacity.
  explicit Array(uint32_t capacity):
      Array(ArrayBlock<T>::allocate(capacity, true, true)) {
    assert(block->capacity >= capacity);
  }

  /// Returns the array identifier.
  ArrayID getID() const { return arrayID; }

  /// Returns the size of the array (capacity).
  uint64_t size() const { return block->capacity; }
  /// Checks whether the array is empty.
  bool empty() const { return size() == 0; }

  /// Returns the begin iterator.
  ArrayIterator<T> begin() const { return ArrayIterator<T>(block, 0); }
  /// Returns the end iterator.
  ArrayIterator<T> end() const { return ArrayIterator<T>(block, block->capacity); }

  T &operator[](size_t i) { return block->items[i]; }
  const T &operator[](size_t i) const { return block->items[i]; }

private:
  /// Array identifier.
  const ArrayID arrayID;
  /// Pointer to the first block of the list.
  ArrayBlock<T> *block;
};

} // namespace eda::gate::model
