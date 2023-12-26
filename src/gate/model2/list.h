//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"

#include <algorithm>
#include <cassert>
#include <cstring>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// List Block
//===----------------------------------------------------------------------===//

/// Block containing a number of elements of type T.
template<typename T>
class ListBlock final : public Object<ListBlock<T>, ListBlockID> {
  friend class Storage<ListBlock<T>>;

  static_assert(sizeof(T) == 8 || sizeof(T) == 16 || sizeof(T) == 32);
  static constexpr size_t MinCapacity = 32 / sizeof(T);

public:
  /// Returns the block size in bytes depending on the capacity.
  static constexpr size_t getSizeInBytes(uint32_t sizeInItems) {
    return sizeInItems <= MinCapacity
        ? ListBlockID::Size
        : ListBlockID::Size + sizeof(T)*(sizeInItems - MinCapacity);
  }

  /// Returns the block capacity depending on the size in bytes.
  static constexpr uint32_t getSizeInItems(size_t sizeInBytes) {
    return sizeInBytes < ListBlockID::Size
        ? 0
        : (sizeInBytes - ListBlockID::Size) / sizeof(T) + MinCapacity;
  }

  /// Allocates a block w/ the specified capacity.
  static inline ListBlockID allocate(uint32_t capacity, bool begin, bool end) {
    const auto sizeInBytes = std::min(getSizeInBytes(capacity), PAGE_SIZE);
    const auto sizeInItems = getSizeInItems(sizeInBytes);
    return allocateExt<ListBlock<T>>(sizeInBytes, sizeInItems, begin, end);
  }

  /// Allocates a block and fills it w/ the given items.
  static inline ListBlockID allocate(
      const std::vector<T> &items, bool begin, bool end) {
    const ListBlockID blockID = allocate(items.size(), begin, end);
    ListBlock<T> *block = access<ListBlock<T>>(blockID);
    assert(block->capacity == items.size());
    for (size_t i = 0; i < items.size(); ++i) {
      block->items[i] = items[i];
    }
    return blockID;
  }

  /// Checks if the item is null.
  static inline bool isNull(const T &item) {
    if constexpr (sizeof(T) == 8) {
      return reinterpret_cast<const uint64_t&>(item) == 0;
    } else {
      const char *data = reinterpret_cast<const char*>(&item);
      if (*data) return false;
      return memcmp(data, data + 1, sizeof(T) - 1) == 0;
    }
  }

  /// Nullifies the item.
  static inline void setNull(T &item) {
    if constexpr (sizeof(T) == 8) {
      reinterpret_cast<uint64_t&>(item) = 0;
    } else {
      memset(&item, 0, sizeof(T));
    }
  }

  /// Returns the pointer to the previous block.
  ListBlock<T> *prevBlock() const {
    return access<ListBlock<T>>(ListBlockID::makeFID(prevBlockSID));
  }

  /// Returns the pointer to the next block.
  ListBlock<T> *nextBlock() const {
    return access<ListBlock<T>>(ListBlockID::makeFID(nextBlockSID));
  }

  /// Number of items in the entire list (for the first block only).
  uint64_t totalSize;
  /// Capacity of the block. 
  uint32_t capacity;
  /// Number of items in the block.
  uint32_t size;
  /// Index of the last occupied item.
  uint32_t last;
  /// SID of the next block (the first block for the final one).
  uint32_t nextBlockSID;
  /// SID of the previous block (the final block for the first one).
  uint32_t prevBlockSID;
  /// First block of the list.
  uint32_t begin : 1;
  /// Final block of the list.
  uint32_t end : 1;
  /// Block items (non-zero elements).
  T items[MinCapacity /* or more */];

private:
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
};

static_assert(sizeof(ListBlock<uint64_t>) == ListBlockID::Size);

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
    assert(block != nullptr);

    if (block->size == 0 || index == block->last) {
      index = 0;
      moveNextBlock();
    } else {
      index++;
    }

    if (block != nullptr) {
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
  T &operator *() {
    assert(block != nullptr);
    return reinterpret_cast<T&>(block->items[index]);
  }

private:
  /// Constructs the list iterator for the given block.
  ListIterator(ListBlockID blockID):
      blockID(blockID),
      index(0),
      block(access<ListBlock<T>>(blockID)) {
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
      block = access<ListBlock<T>>(blockID);
    }
  }

  /// Skips null items.
  void skipNullItems() {
    while (ListBlock<T>::isNull(block->items[index])) index++;
  }

  /// Current block SID.
  ListBlockID blockID;
  /// Current index.
  uint32_t index;
  /// Current block.
  ListBlock<T> *block;
};

//===----------------------------------------------------------------------===//
// List Interface
//===----------------------------------------------------------------------===//

template <typename T>
class List final {
  static constexpr uint32_t DefaultBlockCapacity =
      ListBlock<T>::getSizeInItems(256);

public:
  /// Constructs a wrapper around the given list structure.
  List(ListID listID):
      listID(listID), block(access<ListBlock<T>>(listID)) {
    assert(block != nullptr && block->begin);
  }

  /// Constructs a new list w/ the specified capacity.
  List(uint32_t capacity):
      List(ListBlock<T>::allocate(capacity, true, true)) {
    const auto blockSID = listID.getSID();
    block->prevBlockSID = blockSID;
    block->nextBlockSID = blockSID;
  }

  /// Constructs a new list w/ the default capacity.
  List(): List(DefaultBlockCapacity) {}

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
  void push_back(T value) {
    assert(!ListBlock<T>::isNull(value));

    auto *lastBlock = block->prevBlock();
    assert(lastBlock->end);

    if (lastBlock->last + 1 != lastBlock->capacity) {
      lastBlock->items[++lastBlock->last] = value;
      lastBlock->size++;
    } else {
      auto nextBlockFID = ListBlock<T>::allocate(lastBlock->capacity, 0, 1);
      auto nextBlockSID = nextBlockFID.getSID();

      auto *nextBlock = access<ListBlock<T>>(nextBlockFID);

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
    assert(!ListBlock<T>::isNull(item));

    ListBlock<T>::setNull(item);
    pos.block->size--;
    block->totalSize--;

    if (pos.block->size == 0 && !pos.block->begin) {
      auto *prevBlock = pos.block->prevBlock();
      prevBlock->nextBlockSID = pos.block->nextBlockSID;

      auto *nextBlock = pos.block->nextBlock();
      nextBlock->prevBlockSID = pos.block->prevBlockSID;

      prevBlock->end = pos.block->end;

      release<ListBlock<T>>(pos.blockID);
      return ListIterator<T>(ListBlockID::makeFID(pos.block->nextBlockSID));
    }

    // Update the index of the last occupied item.
    for (uint32_t i = pos.index; i > 0; i--) {
      if (!ListBlock<T>::isNull(pos.block->items[i - 1])) {
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
  ListBlock<T> *block;
};

} // namespace eda::gate::model
