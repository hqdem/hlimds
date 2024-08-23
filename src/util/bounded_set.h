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

namespace eda::util {

template <class NumType = unsigned, class Allocator = std::allocator<NumType>>
class BoundedSet final {
  typedef NumType* iterator;
  static_assert(std::is_integral_v<NumType>, "NumType must be integral");

  typedef uint16_t SizeType;    

private:
  Allocator setAllocator;
  SizeType maxSize;
  SizeType setSize;
  iterator setPtr;
  uint64_t signature;
public:
  /// Copy constructor for BoundedSet.
  BoundedSet(const BoundedSet<NumType, Allocator> &other);

  /**
   * @brief Constructs a new BoundedSet with elements from
   * std::unordered_set with maxSize equal it's size.
   * @param set std::unordered_set with elements for new BoundedSet.
   */
  BoundedSet(const std::unordered_set<NumType> &set);
  /**
   * @brief Constructs a new BoundedSet with elements from
   * std::unordered_set with custom maxSize.
   * @param set std::unordered_set with elements for new BoundedSet.
   * @param maxSize Max amount of elements in new BoundedSet.
   */
  BoundedSet(const std::unordered_set<NumType> &set, const SizeType maxSize);

  /// Constructs BoundedSet without any elements and with fixed size.
  BoundedSet(SizeType maxSize);

  /// Constructs set-singleton with fixed size.
  BoundedSet(SizeType maxSize, NumType singleElement);

  /// Destructor of set.
  ~BoundedSet();

  /**
  * @brief Merges two sets into one, if possible.
  * @return True if the operation was successful, otherwise it is a false.
  */
  bool merge(const BoundedSet<NumType, Allocator> &other);

  /**
  * @brief Checks whether it is possible to merge
  * two sets into one with a valid size.
  */
  bool unionCheck(const BoundedSet<NumType, Allocator> &other);

  /**  
  * @brief Adds a new element to the set if there is
  * a place for it and it is not there yet.
  * @param newElement New element for set.
  * @param isChecked Specifies whether to perform an additional check 
  * for the occurrence of an element in the set.
  * @return True if the element was placed in the set, otherwise it is false.
  */
  bool insert(NumType newElement, bool isChecked = false);
  /**
  * @brief Searches for the location of an element in the set.
  * @param num The target element value.
  * @return Returns a pointer to the targeted element, if it
  * locates in the set. Otherwise, it returns a pointer to the end of the set.
  */
  iterator find(NumType num) const;

  /// Returns the current size of the set.
  SizeType size() const;

  /// Returns a pointer to the last nonexistent element of the set.
  iterator end() const;

  /// Returns a pointer to the first element of the set.
  iterator begin() const;

  /// Checks the set for emptiness.
  bool empty() const;

  /**
  * @brief AAllows to fill an already
  * existing BoundedSet with elements of std::unordered_set.
  */
  BoundedSet<NumType, Allocator> &operator=
      (const std::unordered_set<NumType> &set);

  /// Checks whether two sets are equal.
  bool operator ==(const BoundedSet<NumType, Allocator> &other) const;


  /// Returns minimum value in the set.
  NumType minValue() const;

  /// Returns maximum value in the set.
  NumType maxValue() const;
};

/// Counts the number of bits in the signature
static inline uint16_t countSBits(uint64_t x) {
  x = x - ((x >> 1) & 0x5555555555555555ull);
  x = (x & 0x3333333333333333UL) + ((x >> 2) & 0x3333333333333333ull);
  x = (((x + (x >> 4)) & 0xF0F0F0F0F0F0F0Full) * 0x101010101010101ull) >> 56;
  return static_cast<uint16_t>(x);
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(SizeType maxSize){
  this->maxSize = maxSize;
  this->setSize = 0;
  this->signature = 0;
  this->setPtr = setAllocator.allocate(maxSize);
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(SizeType maxSize,
    NumType singleElement): BoundedSet(maxSize) {
  this->insert(singleElement, 1);
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
  this->signature |= other.signature;
  return true;
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::unionCheck(
    const BoundedSet<NumType, Allocator> &other) {
  if (other.empty()) return true;
  if (this->size() + other.size() <= this->maxSize) return true;
  if (this->empty()) return false;
  if (!(this->signature & other.signature)) return false;
  if (!(this->signature & other.signature) ||
      (this->minValue() > other.maxValue()) || (this->maxValue() < other.minValue())) return false;
  if (countSBits(this->signature | other.signature) > this->maxSize) return false;
  SizeType cnt = 0;
  const SizeType n = this->setSize;
  const SizeType m = other.size();
  SizeType i = 0, j = 0;
  while ((i < n) && (j < m)) {
    if (this->setPtr[i] == other.setPtr[j]) {
      i++;
      j++;
    } else if (this->setPtr[i] < other.setPtr[j]) {
      i++;
    } else {
      j++;
    }
    cnt++;
  }
  cnt+= (n - i) + (m - j);
  return (cnt <= this->maxSize);
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::insert(NumType newElement, bool isChecked) {
  if ((this->setSize + 1 <= this->maxSize) &&
          (isChecked || (this->find(newElement) == this->end()) )) {
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
    this->signature |= (uint64_t) 1 << (newElement % 64);
    return true;
  }
  return false;
}

template <class NumType, class Allocator>
typename BoundedSet<NumType, Allocator>::iterator
BoundedSet<NumType, Allocator>::find(NumType num) const {
  iterator r = std::lower_bound(this->begin(), this->end(), num);
  if (*r == num) return r;
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
  return (this->setSize == 0);
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::~BoundedSet() {
  setAllocator.deallocate(setPtr, maxSize);
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
    this->insert(i, 1);
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
    const BoundedSet<NumType, Allocator> &other) : BoundedSet(other.maxSize) {
  for (auto i : other)
    this->insert(i, 1);
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(
    const std::unordered_set<NumType> &set) : BoundedSet(set.size()) {
  for (auto i : set) 
    this->insert(i, 1);
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
    this->insert(i, 1);
}

template <class NumType, class Allocator>
NumType BoundedSet<NumType, Allocator>::minValue() const {
  assert(!this->empty());
  return this->setPtr[0];
}

template <class NumType, class Allocator>
NumType BoundedSet<NumType, Allocator>::maxValue() const {
  assert(!this->empty());
  return this->setPtr[this->size() - 1];
}

} // namespace eda::util
