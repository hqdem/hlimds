//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"
#include "gate/model2/storage.h"

#include <cassert>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// List Block
//===----------------------------------------------------------------------===//

#pragma pack(push, 1)
struct ListBlock final {
  using ID = ListBlockID;

  /// Initial block of the list.
  uint64_t begin : 1;
  /// Final block of the list.
  uint64_t end : 1;
  /// Number of items in the list (starting from this block).
  uint64_t totalSize : 62;
  /// Capacity of this block. 
  uint32_t capacity;
  /// Number of items in this block.
  uint32_t size;
  /// Short identifier of the next block (the start block for the final one).
  uint32_t nextBlockSID;
  /// Short identifier of the previous block.
  uint32_t prevBlockSID;
  /// Block items (64-bit elements).
  uint64_t elements[1 /* ... */];
};
#pragma pack(pop)

static_assert(sizeof(ListBlock) == ListBlockID::Size);

//===----------------------------------------------------------------------===//
// List Iterator
//===----------------------------------------------------------------------===//

template <typename T>
class List;

template <typename T>
class ListIterator final {
  friend class List<T>;

public:
  ListIterator(const ListIterator<T> &) = default;
  ListIterator &operator =(const ListIterator<T> &) = default;

  bool operator ==(const ListIterator<T> &r) const {
    return block == r.block && index == r.index && count == r.count;
  }

  /// Prefix increment operator.
  ListIterator<T> &operator ++() {
    assert(block != nullptr);

    if (count == block->size) {
      index = count = 0;

      if (block->end) {
        block = nullptr;
        return *this;
      }

      block = access<ListBlock>(ListBlockID::makeFID(block->nextBlockSID));
      assert(block->size);

      while (block->elements[index] == OBJ_NULL_ID) index++;
    }

    count++;

    return *this;
  }

  /// Postfix increment operator.
  ListIterator<T> operator ++(int) {
    auto temp = *this;
    ++*this;
    return temp;
  }

  typename T::ID &operator *() {
    return block->elements[index];
  }

private:
  /// Constructs the list iterator for the given block.
  ListIterator(ListBlock *block):
      block(block), index(0), count(0) {
    if (block->size) {
      while (block->elements[index] == OBJ_NULL_ID) index++;
    } else {
      block = nullptr;
    }
  }

  /// Current block.
  ListBlock *block;
  /// Current item index.
  uint32_t index;
  /// Number of items in the blocks.
  uint32_t count; 
};

/// List interface.
template <typename T>
class List final {
public:
  List(ListID listID):
      listID(listID), block(access<ListBlock>(listID)) {
    assert(block != nullptr);
  }

  /// Returns the size of the list.
  uint64_t size() const { return block->totalSize; }
  /// Checks whether the list is empty.
  bool empty() const { return size() == 0; }

  /// Returns the begin iterator.
  ListIterator<T> begin() const { return ListIterator<T>(block); }
  /// Returns the end iterator.
  ListIterator<T> end() const { return ListIterator<T>(nullptr); }

  /// Adds the specified element to the end of the list.
  void push_back(typename T::ID value) {
    // TODO: To be implemented.
  }

  /// Erases the specified element from the list.
  ListIterator<T> erase(ListIterator<T> pos) {
    // TODO: To be implemented.
    return pos;
  }

  /// Inserts the specified element before the specified location in the list.
  ListIterator<T> insert(ListIterator<T> pos, typename T::ID value) {
    // TODO: To be implemented.
    return pos;
  }
  
private:
  /// List identifier.
  const ListID listID;
  /// Pointer to the first block of the list.
  const ListBlock *block;
};

} // namespace eda::gate::model
