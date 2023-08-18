//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// List Block
//===----------------------------------------------------------------------===//

struct ListBlock final {
  uint64_t totalSize;
  uint32_t blockSize;
  uint32_t blockBusy;
  uint32_t nextBlockSID;
  uint32_t prevBlockSID;
  uint64_t objectIDs[1 /* ... */];
};

static_assert(sizeof(ListBlock) == ListID::Size);

//===----------------------------------------------------------------------===//
// List Iterator
//===----------------------------------------------------------------------===//

class List;

class ListIterator final {
  friend class List;

public:
  ListIterator(const ListIterator &) = default;
  ListIterator &operator =(const ListIterator &) = default;

  bool operator ==(const ListIterator &r) const {
    return block == r.block && index == r.index && count == r.count;
  }

  ListIterator &operator ++() {
    // TODO: To be implemented.
    return *this;
  }

  ListIterator operator ++(int) {
    auto temp = *this;
    ++*this;
    return temp;
  }

private:
  ListIterator(ListBlock *block):
    block(block), index(0), count(0) {}

  ListBlock *block;
  uint32_t index;
  uint32_t count; 
};

/// List interface.
class List final {
public:
  List(ListID listID):
    listID(listID), block(nullptr) {}

  /// Returns the size of the list.
  uint64_t getSize() const { return block->totalSize; }

  /// Checks whether the list is empty.
  bool isEmpty() const { return getSize() == 0; }


private:
  /// List identifier.
  const ListID listID;

  ListBlock *block;
};

} // namespace eda::gate::model
