//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <unordered_set>

template <class NumType = unsigned, class Allocator = std::allocator<NumType>>
class BoundedSet final {
public:
  using iterator = NumType*;
  using SizeType = uint16_t;

  /**
   * @brief Constructs a new BoundedSet with elements from
   * std::unordered_set with maxSize equal it's size
   * @param set std::unordered_set with elements for new BoundedSet.
   */
  BoundedSet(const std::unordered_set<NumType> &set);
  /**
   * @brief Constructs a new BoundedSet with elements from
   * std::unordered_set with custom maxSize
   * @param set std::unordered_set with elements for new BoundedSet
   * @param maxSize max amount of elements in new BoundedSet.
   */
  BoundedSet(const std::unordered_set<NumType> &set, const SizeType maxSize);
  /// Constructs BoundedSet without any elements and with fixed size.
  BoundedSet(SizeType maxSize);
  /// Constructs set-singleton with fixed size.
  BoundedSet(SizeType maxSize, NumType singleElement);
  /// Allows to assign already existing BoundedSet with elements of another std::unoredred_set
  BoundedSet<NumType, Allocator> &operator=(const std::unordered_set<NumType> &set);
  /// Copy constructor for BoundedSet
  BoundedSet(const BoundedSet<NumType, Allocator> &other);
  /// Destructor of set
  ~BoundedSet();

  /// returns the signature of set.
  uint64_t getSign() const;
  /// sets the new signature for collections.
  void setSign(uint64_t sign);
  /// Merges two BoundedSets in one. Target of method becomes union if it can.
  bool merge(const BoundedSet<NumType, Allocator> &other);
  /// Checks if the union of two sets has size less than maxSize of parent sets.
  bool unionCheck(const BoundedSet<NumType, Allocator> &other);
  /// @brief add new element in set if it is not located in this set previously
  /// @param newElement new element for collection
  /// @return true if element is inserted or false if there is no space in collection
  bool insert(NumType newElement);
  /// @brief add new element without check
  /// @param newElement new element for collection
  /// @return true if element is inserted or false if there is no space in collection
  bool checkedInsert(NumType newElement);
  /// @brief find place of target element
  /// @param num target element
  /// @return a pointer on target element in collection if it is here. Else return end of set.
  iterator find(NumType num) const;
  /// Returns the current size of set
  SizeType size() const;
  /// Returns the pointer on unexisting last element of set
  iterator end() const;
  /// Returns pointer on first element of set
  iterator begin() const;
  /// Checks set if it is empty
  bool empty() const;
  /// Returns count of target element in set (no more than one in this collection)
  SizeType count(NumType target) const;
  /// Allows to compare two BoundedSets
  bool operator==(const BoundedSet<NumType, Allocator> &other) const;
  /// Allows to compare BoundedSet with unordered_set
  bool operator==(const std::unordered_set<NumType> &set) const;

private:
  Allocator setAllocator;
  SizeType maxSize;
  SizeType setSize;
  iterator setPtr;
  uint64_t signature;
};

template <class NumType, class Allocator>
void BoundedSet<NumType, Allocator>::setSign(uint64_t sign) {
  this->signature = sign;
}

template <class NumType, class Allocator>
uint64_t BoundedSet<NumType, Allocator>::getSign() const{
  return this->signature;
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(SizeType maxSize) {
  this->maxSize = maxSize;
  this->setSize = 0;
  this->signature = 0;
  this->setPtr = setAllocator.allocate(maxSize);
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(SizeType maxSize,
                                       NumType singleElement) {
  this->maxSize = maxSize;
  this->setSize = 0;
  this->signature = 0;
  this->setPtr = setAllocator.allocate(maxSize);
  this->checkedInsert(singleElement);
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::merge(
    const BoundedSet<NumType, Allocator> &other) {
  if (!this->unionCheck(other)) return false;
  NumType* newPtr = this->setAllocator.allocate(this->maxSize);
  const SizeType n = this->setSize;
  const SizeType m = other.size();
  SizeType i = 0;
  SizeType j = 0;
  SizeType cnt = 0;
  while ((i < n) && (j < m)) {
    if (this->setPtr[i] == other.setPtr[j]) {
      newPtr[cnt] = this->setPtr[i];
      cnt++;
      i++;
      j++;
    }
    else {
      if (this->setPtr[i] < other.setPtr[j]) {
        newPtr[cnt] = this->setPtr[i];
        cnt++;
        i++;
      } else {
        newPtr[cnt] = other.setPtr[j];
        cnt++;
        j++;
      }
    }
  }
  while (i < n) {
    newPtr[cnt] = this->setPtr[i];
    cnt++;
    i++;
  }
  while (j < m) {
    newPtr[cnt] = other.setPtr[j];
    cnt++;
    j++;
  }
  setAllocator.deallocate(this->setPtr, this->maxSize);
  this->setPtr = newPtr;
  this->setSize = cnt;
  this->setSign(this->signature | other.getSign());
  return true;
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::unionCheck(
    const BoundedSet<NumType, Allocator> &other) {
  SizeType cnt = 0;
  const SizeType n = this->setSize;
  const SizeType m = other.size();
  SizeType i = 0, j = 0;
  while ((i < n) && (j < m)) {
    if (this->setPtr[i] == other.setPtr[j]) {
      cnt++;
      i++;
      j++;
    }
    else {
      if (this->setPtr[i] < other.setPtr[j]) {
        cnt++;
        i++;
      } else {
        cnt++;
        j++;
      }
    }
  }
  while (i < n) {
    cnt++;
    i++;
  }
  while (j < m) {
    cnt++;
    j++;
  }
  return (cnt <= this->maxSize);
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::insert(NumType newElement) {
  if ((this->setSize + 1 <= this->maxSize) &&
                                 (this->find(newElement) == this->end())) {
    SizeType i = 0;
    while ((this->setPtr[i] < newElement) && (i < this->setSize)) 
      i++;
    SizeType j = this->setSize;
    while ((j > i)) {
      this->setPtr[j] = this->setPtr[j - 1];
      j--;
    }
    this->setPtr[i] = newElement;
    this->setSize++;
    return true;
  }
  return false;
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::checkedInsert(NumType newElement) {
  if (this->setSize + 1 <= this->maxSize) {
    SizeType i = 0;
    while ((this->setPtr[i] < newElement) && (i < this->setSize)) 
      i++;
    SizeType j = this->setSize;
    while (j > i) {
      this->setPtr[j] = this->setPtr[j - 1];
      j--;
    }
    this->setPtr[i] = newElement;
    this->setSize++;
    return true;
  }
  return false;
}

template <class NumType, class Allocator>
typename BoundedSet<NumType, Allocator>::iterator
    BoundedSet<NumType, Allocator>::find(NumType num) const {
  SizeType l = 0;
  SizeType r = this->size();
  SizeType mid;
  if (r == 0) return this->end();
  while ((l < r) && (r > 1)) {
    mid = (l + r) / 2;
    if (this->setPtr[mid] > num) r = mid;
    else l = mid + 1;
  }
  r--;
  if (this->setPtr[r] == num) return this->setPtr + r;
  return this->end();
}

template <class NumType, class Allocator>
typename BoundedSet<NumType, Allocator>::SizeType
    BoundedSet<NumType, Allocator>::size() const {
  return this->setSize;
}

template <class NumType, class Allocator>
typename BoundedSet<NumType, Allocator>::iterator
    BoundedSet<NumType, Allocator>::end() const {
  return this->setPtr + this->setSize;
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::empty() const {
  return  (this->setSize == 0);
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::~BoundedSet() {
  setAllocator.deallocate(setPtr, maxSize);
}

template <class NumType, class Allocator>
typename BoundedSet<NumType, Allocator>::SizeType
    BoundedSet<NumType, Allocator>::count(NumType target) const {
  SizeType result = 0;
  for (auto i = 0; i < this->size(); i++) { 
    if (target == this->begin()[i]) 
      result++;
  }
  return result;
}

template <class NumType, class Allocator>
typename BoundedSet<NumType, Allocator>::iterator
    BoundedSet<NumType, Allocator>::begin() const {
  return this->setPtr;
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>& BoundedSet<NumType, Allocator>::operator=(
    const std::unordered_set<NumType> &set) {
  setAllocator.deallocate(this->setPtr, maxSize);
  this->maxSize = set.size();
  this->setPtr = setAllocator.allocate(this->maxSize);
  this->setSize = 0;
  this->signature = 0;
  for (auto i : set) 
    this->checkedInsert(i);
  return  *this;
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::operator==(
    const BoundedSet<NumType, Allocator> &other) const {
  bool result = 1;
  if (this->size() != other.size()) result = 0;
  for (auto i : other)
    result = result && (this->find(i) != this->end());
  return result;
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(
    const BoundedSet<NumType, Allocator> &other) {
  this->maxSize = other.maxSize;
  this->setPtr = setAllocator.allocate(this->maxSize);
  this->setSize = 0;
  this->signature = other.getSign();
  for (auto i : other)
    this->checkedInsert(i);
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(
    const std::unordered_set<NumType>&set) {
  this->maxSize = set.size();
  this->setPtr = setAllocator.allocate(this->maxSize);
  this->setSize = 0;
  this->signature = 0;
  for (auto i : set) 
    this->checkedInsert(i);
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(
    const std::unordered_set<NumType> &set, const SizeType maxSize) {
  assert(set.size() <= maxSize);
  this->maxSize = maxSize;
  this->setPtr = setAllocator.allocate(this->maxSize);
  this->setSize = 0;
  this->signature = 0;
  for (auto i : set) 
    this->checkedInsert(i);
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::operator==(
    const std::unordered_set<NumType> &set) const {
  bool result = 1;
  if (this->size() != set.size()) result = 0;
  for (auto i : set)
    result = result && (this->find(i) != this->end());
  return result;
}
