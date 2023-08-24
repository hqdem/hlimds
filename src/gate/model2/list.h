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

/// Block containing a number of 64-bit elements (FIDs != OBJ_NULL_ID).
struct ListBlock final {
  using ID = ListBlockID;

  using ElementType = uint64_t;
  static_assert(sizeof(ElementType) == 8);

  static constexpr size_t MIN_CAPACITY = 4;

  /// Returns the block size in bytes depending on the capacity.
  static constexpr size_t getSizeInBytes(uint32_t sizeInItems) {
    return sizeInItems <= MIN_CAPACITY
        ? ID::Size
        : ID::Size + sizeof(ElementType)*(sizeInItems - MIN_CAPACITY);
  }

  /// Returns the block capacity depending on the size in bytes.
  static constexpr uint32_t getSizeInItems(size_t sizeInBytes) {
    return sizeInBytes < ID::Size
        ? 0
        : (sizeInBytes - ID::Size) / sizeof(ElementType) + MIN_CAPACITY;
  }

  /// Allocates the block w/ the specified capacity.
  static ID allocate(uint32_t capacity, bool begin, bool end) {
    auto sizeInBytes = std::min(getSizeInBytes(capacity), PAGE_SIZE);
    auto sizeInItems = getSizeInItems(sizeInBytes);
    return allocateExt<ListBlock>(sizeInBytes, sizeInItems, begin, end);
  }

  /// Constructs a block w/ the specified capacity and flags.
  ListBlock(uint32_t capacity, bool begin, bool end):
      totalSize(0),
      capacity(capacity),
      size(0),
      last(-1),
      nextBlockSID(0),
      prevBlockSID(0),
      begin(begin),
      end(end) {
    assert(capacity != 0);
  }

  /// Returns the pointer to the previous block.
  ListBlock *prevBlock() const {
    return access<ListBlock>(ListBlockID::makeFID(prevBlockSID));
  }

  /// Returns the pointer to the next block.
  ListBlock *nextBlock() const {
    return access<ListBlock>(ListBlockID::makeFID(nextBlockSID));
  }

  /// Number of items in the entire list (for the first block).
  uint64_t totalSize;
  /// Capacity of the block. 
  uint32_t capacity;
  /// Number of items in the block.
  uint32_t size;
  /// Index of the last occupied item of the block.
  uint32_t last;
  /// SID of the next block (the first block for the final one).
  uint32_t nextBlockSID;
  /// SID of the previous block (the final block for the first one).
  uint32_t prevBlockSID;
  /// First block of the list.
  uint32_t begin : 1;
  /// Final block of the list.
  uint32_t end : 1;
  /// Block items (64-bit elements != OBJ_NULL_ID).
  ElementType items[MIN_CAPACITY /* or more */];
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
    return blockID == r.blockID && index == r.index;
  }

  bool operator !=(const ListIterator<T> &r) const {
    return !(*this == r);
  }

  /// Prefix increment operator.
  ListIterator<T> &operator ++() {
    assert(blockID != OBJ_NULL_ID);

    if (block->size == 0 || index == block->last) {
      index = 0;
      moveNextBlock();
    } else {
      index++;
    }

    if (blockID != OBJ_NULL_ID) {
      skipNullItems();
    }

    return *this;
  }

  /// Postfix increment operator.
  ListIterator<T> operator ++(int) {
    auto temp = *this;
    ++*this;
    return temp;
  }

  /// Dereferencing operator.
  typename T::ID &operator *() {
    return reinterpret_cast<typename T::ID&>(block->items[index]);
  }

private:
  /// Constructs the list iterator for the given block.
  ListIterator(ListBlockID blockID):
      blockID(blockID),
      index(0),
      block(access<ListBlock>(blockID)) {
    while (block != nullptr && block->size == 0) {
      moveNextBlock();
    }

    if (block != nullptr && block->size > 0) {
      skipNullItems();
    }
  }

  /// Moves to the next block.
  void moveNextBlock() {
    if (block->end) {
      blockID = OBJ_NULL_ID;
      block = nullptr;
    } else {
      blockID = ListBlockID::makeFID(block->nextBlockSID);
      block = access<ListBlock>(blockID);
    }
  }

  /// Skips null items.
  void skipNullItems() {
    while (block->items[index] == OBJ_NULL_ID) {
      index++;
    }
  }

  /// Current block SID.
  ListBlockID blockID;
  /// Current index.
  uint32_t index;
  /// Current block.
  ListBlock *block;
};

//===----------------------------------------------------------------------===//
// List Interface
//===----------------------------------------------------------------------===//

template <typename T>
class List final {
  static constexpr uint32_t DEFAULT_BLOCK_CAPACITY = ListBlock::getSizeInItems(256);

public:
  /// Constructs a wrapper around the given list structure.
  List(ListID listID):
      listID(listID), block(access<ListBlock>(listID)) {
    assert(block != nullptr);
  }

  /// Constructs a new list w/ the specified capacity.
  List(uint32_t capacity, bool begin, bool end):
      List(ListBlock::allocate(capacity, begin, end)) {
    uint32_t blockSID = listID.getSID();
    block->prevBlockSID = blockSID;
    block->nextBlockSID = blockSID;
  }

  /// Constructs a new list w/ the default capacity.
  List(): List(DEFAULT_BLOCK_CAPACITY, true, true) {}

  /// Returns the list identifier.
  ListID getID() const { return listID; }

  /// Returns the size of the list.
  uint64_t size() const { return block->totalSize; }
  /// Checks whether the list is empty.
  bool empty() const { return size() == 0; }

  /// Returns the begin iterator.
  ListIterator<T> begin() const { return ListIterator<T>(listID); }
  /// Returns the end iterator.
  ListIterator<T> end() const { return ListIterator<T>(OBJ_NULL_ID); }

  /// Adds the specified element to the end of the list.
  void push_back(typename T::ID value) {
    assert(value != OBJ_NULL_ID);

    auto *lastBlock = block->prevBlock();
    assert(lastBlock->end);

    if (lastBlock->last + 1 != lastBlock->capacity) {
      lastBlock->items[++lastBlock->last] = value;
      lastBlock->size++;
    } else {
      auto nextBlockFID = ListBlock::allocate(lastBlock->capacity, 0, 1);
      auto nextBlockSID = nextBlockFID.getSID();

      auto *nextBlock = access<ListBlock>(nextBlockFID);

      nextBlock->nextBlockSID = lastBlock->nextBlockSID;
      nextBlock->prevBlockSID = block->prevBlockSID;
      lastBlock->nextBlockSID = block->prevBlockSID = nextBlockSID;

      lastBlock->end = 0;

      nextBlock->items[++nextBlock->last] = value;
      nextBlock->size++;
    }

    block->totalSize++;
  }

  /// Erases the specified element from the list.
  ListIterator<T> erase(ListIterator<T> pos) {
    assert(pos.block != nullptr);

    auto &item = pos.block->items[pos.index];
    assert(item != OBJ_NULL_ID);

    item = OBJ_NULL_ID;
    pos.block->size--;
    block->totalSize--;

    if (pos.block->size == 0 && !pos.block->begin) {
      auto *prevBlock = pos.block->prevBlock();
      prevBlock->nextBlockSID = pos.block->nextBlockSID;

      auto *nextBlock = pos.block->nextBlock();
      nextBlock->prevBlockSID = pos.block->prevBlockSID;

      prevBlock->end = pos.block->end;

      release<ListBlock>(pos.blockID);
      return ListIterator<T>(ListBlockID::makeFID(pos.block->nextBlockSID));
    }

    // Update the index of the last occupied item.
    for (uint32_t i = pos.index; i > 0; i--) {
      if (pos.block->items[i - 1] != OBJ_NULL_ID) {
        pos.block->last = i - 1;
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
