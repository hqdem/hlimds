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

#include <algorithm>
#include <cassert>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// List Block
//===----------------------------------------------------------------------===//

struct ListBlock final {
  using ID = ListBlockID;

  static constexpr size_t MIN_CAPACITY = 4;

  /// Returns the block size in bytes depending on the capacity.
  static constexpr size_t getSizeInBytes(uint32_t sizeInItems) {
    return sizeInItems <= MIN_CAPACITY
        ? ID::Size
        : ID::Size + sizeof(uint64_t)*(sizeInItems - MIN_CAPACITY);
  }

  /// Returns the block capacity depending on the size in bytes.
  static constexpr uint32_t getSizeInItems(size_t sizeInBytes) {
    return sizeInBytes < ID::Size
        ? 0
        : (sizeInBytes - ID::Size) / sizeof(uint64_t) + MIN_CAPACITY;
  }

  /// Allocates the block w/ the specified capacity.
  static ID allocate(uint32_t capacity) {
    auto sizeInBytes = std::min(getSizeInBytes(capacity), PAGE_SIZE);
    auto sizeInItems = getSizeInItems(sizeInBytes);
    return allocateExt<ListBlock>(sizeInBytes, sizeInItems);
  }

  /// Constructs a list block.
  ListBlock(uint32_t capacity):
      totalSize(0),
      capacity(capacity),
      size(0),
      last(-1),
      nextBlockSID(OBJ_NULL_ID),
      prevBlockSID(OBJ_NULL_ID),
      begin(0),
      end(0) {}

  /// Returns the pointer to the previous block.
  ListBlock *prevBlock() const {
    return access<ListBlock>(ListBlockID::makeFID(prevBlockSID));
  }

  /// Returns the pointer to the next block.
  ListBlock *nextBlock() const {
    return access<ListBlock>(ListBlockID::makeFID(nextBlockSID));
  }

  /// Number of items in the list (for the first block).
  uint64_t totalSize;
  /// Capacity of this block. 
  uint32_t capacity;
  /// Number of items in this block.
  uint32_t size;
  /// Last occupied item of this block.
  uint32_t last;
  /// Short identifier of the next block (the start block for the final one).
  uint32_t nextBlockSID;
  /// Short identifier of the previous block.
  uint32_t prevBlockSID;
  /// Initial block of the list.
  uint32_t begin : 1;
  /// Final block of the list.
  uint32_t end : 1;
  /// Block items (64-bit elements).
  uint64_t items[MIN_CAPACITY /* or more */];
};

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

  bool operator !=(const ListIterator<T> &r) const {
    return !(*this == r);
  }

  /// Prefix increment operator.
  ListIterator<T> &operator ++() {
    assert(block != nullptr);

    if (count == block->size - 1) {
      index = count = 0;

      if (block->end) {
        block = nullptr;
        return *this;
      }

      block = block->nextBlock();
      assert(block->size);

      while (block->items[index] == OBJ_NULL_ID) index++;
    }

    count++;
    index++;

    while (block->items[index] == OBJ_NULL_ID) index++;

    return *this;
  }

  /// Postfix increment operator.
  ListIterator<T> operator ++(int) {
    auto temp = *this;
    ++*this;
    return temp;
  }

  typename T::ID &operator *() {
    return reinterpret_cast<typename T::ID&>(block->items[index]);
  }

private:
  /// Constructs the list iterator for the given block.
  ListIterator(ListBlock *block):
      block(block), index(0), count(0) {
    if (block != nullptr && block->size > 0) {
      while (block->items[index] == OBJ_NULL_ID) index++;
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
  static constexpr uint32_t DEFAULT_BLOCK_CAPACITY = ListBlock::getSizeInItems(256);

public:
  /// Constructs a wrapper around the existing list structure.
  List(ListID listID):
      listID(listID), block(access<ListBlock>(listID)) {
    assert(block != nullptr);
  }

  /// Constructs a new list w/ the specified capacity.
  List(uint32_t capacity):
      List(ListBlock::allocate(capacity)) {
    block->begin = block->end = 1;
    block->prevBlockSID = listID.getSID();
    block->nextBlockSID = listID.getSID();
  }

  /// Constructs a new list w/ the default capacity.
  List(): List(DEFAULT_BLOCK_CAPACITY) {}

  /// Returns the list identifier.
  ListID getListID() const { return listID; }

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
    auto *lastBlock = block->prevBlock();
    assert(lastBlock->end);

    if (lastBlock->last != lastBlock->capacity - 1) {
      lastBlock->items[++lastBlock->last] = value;
      lastBlock->size++;
    } else {
      auto nextBlockFID = ListBlock::allocate(lastBlock->capacity);
      auto nextBlockSID = nextBlockFID.getSID();

      auto *nextBlock = access<ListBlock>(nextBlockFID);

      nextBlock->nextBlockSID = lastBlock->nextBlockSID;
      nextBlock->prevBlockSID = block->prevBlockSID;
      lastBlock->nextBlockSID = block->prevBlockSID = nextBlockSID;

      lastBlock->end = 0;
      nextBlock->end = 1;

      nextBlock->items[++lastBlock->last] = value;
      nextBlock->size++;
    }
  }

  /// Erases the specified element from the list.
  ListIterator<T> erase(ListIterator<T> pos) {
    auto *block = pos->block;
    assert(block != nullptr);

    uint64_t &item = block->items[pos->index];
    assert(item != OBJ_NULL_ID);

    item = OBJ_NULL_ID;
    block->size--;

    if (block->size == 0 && !block->begin) {
      auto *prevBlock = block->prevBlock();
      prevBlock->nextBlockSID = block->nextBlockSID;

      auto *nextBlock = block->nextBlock();
      nextBlock->prevBlockSID = block->prevBlockSID;

      release<ListBlock>(block);
      return ListIterator<T>(nextBlock);
    }

    // Update the index of the last occupied item.
    for (uint32_t i = pos->index; i > 0; i--) {
      if (block->items[i - 1] != OBJ_NULL_ID) {
        block->last = i - 1;
        break;
      }
    }

    return ++pos;
  }

private:
  /// List identifier.
  const ListID listID;
  /// Pointer to the first block of the list.
  ListBlock *block;
};

} // namespace eda::gate::model
