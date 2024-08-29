//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "bits.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_set>

namespace eda::util {

template <class NumType, class Allocator = std::allocator<NumType>>
class BoundedSet final {
  static_assert(std::is_integral_v<NumType>, "NumType must be integral");

  typedef NumType* iterator;
  typedef uint16_t SizeType;    

private:
  static constexpr auto CheckSignature = true;
  static constexpr auto CheckElemRange = false;
  static constexpr auto InPlaceSetSize = 1;

  [[no_unique_address]] Allocator setAllocator;

  const SizeType maxSize;

  const bool isImmutable;
  const uint8_t __padding{0};

  NumType small[InPlaceSetSize];

  SizeType offset;
  SizeType setSize;
  iterator setPtr;
  uint64_t signature;

public:
  /// Constructs an empty set w/ the given size bound.
  BoundedSet(SizeType maxSize, bool isImmutable);

  /// Constructs a singleton-set w/ the given size bound.
  BoundedSet(SizeType maxSize, NumType singleElement, bool isImmutable);

  /**
   * @brief Constructs a new BoundedSet with elements from
   * std::unordered_set with custom maxSize.
   * @param maxSize Max amount of elements in new BoundedSet.
   * @param set std::unordered_set with elements for new BoundedSet.
   */
  BoundedSet(SizeType maxSize,
             const std::unordered_set<NumType> &set,
             bool isImmutable);

  BoundedSet(const BoundedSet<NumType, Allocator> &other);
  BoundedSet(BoundedSet<NumType, Allocator> &&other);

  ~BoundedSet();

  /// Returns the maximum size of the set.
  SizeType capacity() const { return maxSize; }
  /// Returns the size of the set.
  SizeType size() const { return setSize; }
  /// Checks if the set is empty.
  bool empty() const { return setSize == 0; }

  /// Returns the pointer to the first element of the set.
  iterator begin() const { return setPtr; }
  /// Returns the pointer to the element following the last one.
  iterator end() const { return setPtr + setSize; }

  /// Returns the minimum value in the set.
  NumType minValue() const { return setPtr[0]; }
  /// Returns the maximum value in the set.
  NumType maxValue() const { return setPtr[setSize - 1]; }

  /// Checks whether this set contains the other as a subset.
  bool contains(const BoundedSet<NumType, Allocator> &other) const;

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

  /**
   * @brief AAllows to fill an already
   * existing BoundedSet with elements of std::unordered_set.
   */
  BoundedSet<NumType, Allocator> &operator=
      (const std::unordered_set<NumType> &set);

  /// Checks whether two sets are equal.
  bool operator==(const BoundedSet<NumType, Allocator> &other) const;

private:
  bool insertRaw(NumType newElement, bool isChecked);

  void allocate(SizeType maxSize, SizeType setSize, bool isImmutable);
  void deallocate();

  template<auto begin, auto end>
  void copyInPlaceSet(const BoundedSet<NumType, Allocator> &other) {
    if constexpr (begin < end) {
      this->small[begin] = other.small[begin];
      copyInPlaceSet<begin + 1, end>(other);
    }
  }
};

/// Calculates the signature of the given element.
template <class NumType>
static inline uint64_t getSignature(NumType elem) {
  return 1ull << (elem % 64);
}

template <class NumType, class Allocator>
inline void BoundedSet<NumType, Allocator>::allocate(
    SizeType maxSize, SizeType setSize, bool isImmutable) {
  if (!isImmutable) {
    setPtr = setAllocator.allocate(2 * maxSize);
  } else if (setSize <= InPlaceSetSize) {
    setPtr = small;
  } else {
    setPtr = setAllocator.allocate(setSize);
  }
}

template <class NumType, class Allocator>
inline void BoundedSet<NumType, Allocator>::deallocate() {
  if (setPtr != small) {
    auto size = isImmutable ? setSize : 2 * maxSize;
    setAllocator.deallocate(setPtr - offset, size);
  }
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(SizeType maxSize, bool isImmutable):
    maxSize(maxSize),
    isImmutable(isImmutable),
    offset(0),
    setSize(0),
    signature(0) {
  assert(maxSize > 0);
  allocate(maxSize, 0, isImmutable);
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(
    SizeType maxSize, NumType singleElement, bool isImmutable):
    maxSize(maxSize),
    isImmutable(isImmutable),
    offset(0),
    setSize(1),
    signature(getSignature(singleElement)) {
  assert(maxSize > 0);
  allocate(maxSize, 1, isImmutable);
  this->setPtr[0] = singleElement;
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(
    const SizeType maxSize,
    const std::unordered_set<NumType> &set,
    bool isImmutable):
    maxSize(maxSize),
    isImmutable(isImmutable),
    offset(0),
    setSize(0),
    signature(0) {
  assert(maxSize > 0);
  assert(set.size() <= maxSize);

  allocate(maxSize, set.size(), isImmutable);
  for (auto elem : set) 
    this->insertRaw(elem, true);
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(
    const BoundedSet<NumType, Allocator> &other):
    maxSize(other.maxSize),
    isImmutable(other.isImmutable),
    offset(0),
    setSize(other.setSize),
    signature(other.signature) {
  allocate(other.maxSize, other.setSize, other.isImmutable);
  std::memcpy(this->setPtr, other.setPtr, this->setSize * sizeof(NumType));
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::BoundedSet(
    BoundedSet<NumType, Allocator> &&other):
    maxSize(other.maxSize),
    isImmutable(other.isImmutable),
    offset(other.offset),
    setSize(other.setSize),
    setPtr(other.setPtr == other.small ? this->small : other.setPtr),
    signature(other.signature) {
  other.setPtr = other.small;
  copyInPlaceSet<0, InPlaceSetSize>(other);
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::contains(
    const BoundedSet<NumType, Allocator> &other) const {
  if (other.setSize > this->setSize)
    return false;

  if constexpr (CheckSignature) {
    if (this->signature != (this->signature | other.signature))
      return false;
  }

  if constexpr (CheckElemRange) {
    if ((this->minValue() > other.minValue()) ||
        (this->maxValue() < other.maxValue()))
      return false;
  }

  for (const auto elem : other) {
    if (this->find(elem) == this->end())
      return false;
  }

  return true;
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::merge(
    const BoundedSet<NumType, Allocator> &other) {
  assert(!isImmutable);

  if (!this->unionCheck(other)) return false;

  auto *newPtr = this->setPtr;
  newPtr -= this->offset;
  this->offset = (this->offset + this->maxSize) % (2 * this->maxSize);
  newPtr += this->offset;

  const SizeType n = this->setSize;
  const SizeType m = other.setSize;

  SizeType i = 0, j = 0, k = 0;
  while ((i < n) && (j < m)) {
    if (this->setPtr[i] == other.setPtr[j]) {
      newPtr[k++] = this->setPtr[i++];
      j++;
    } else if (this->setPtr[i] < other.setPtr[j]) {
      newPtr[k++] = this->setPtr[i++];
    } else {
      newPtr[k++] = other.setPtr[j++];
    }
  }
  while (i < n) {
    newPtr[k++] = this->setPtr[i++];
  }
  while (j < m) {
    newPtr[k++] = other.setPtr[j++];
  }

  this->setPtr = newPtr;
  this->setSize = k;
  this->signature |= other.signature;

  return true;
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::unionCheck(
    const BoundedSet<NumType, Allocator> &other) {
  if (this->setSize + other.setSize <= this->maxSize)
    return true;

  if constexpr (CheckSignature) {
    if (!(this->signature & other.signature))
      return false;
    if (count_units(this->signature | other.signature) > this->maxSize)
      return false;
  }

  if constexpr (CheckElemRange) {
    if ((this->minValue() > other.maxValue()) ||
        (this->maxValue() < other.minValue()))
      return false;
  }

  const SizeType n = this->setSize;
  const SizeType m = other.setSize;

  SizeType i = 0, j = 0, k = 0;
  while ((i < n) && (j < m)) {
    if (this->setPtr[i] == other.setPtr[j]) {
      i++;
      j++;
    } else if (this->setPtr[i] < other.setPtr[j]) {
      i++;
    } else {
      j++;
    }
    k++;
  }
  k += (n - i) + (m - j);

  return (k <= this->maxSize);
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::insertRaw(
    NumType newElement, bool isChecked) {
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
    this->signature |= getSignature(newElement);
    return true;
  }

  return false;
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::insert(
    NumType newElement, bool isChecked) {
  assert(!isImmutable);
  return insertRaw(newElement, isChecked);
}
 
template <class NumType, class Allocator>
typename BoundedSet<NumType, Allocator>::iterator
BoundedSet<NumType, Allocator>::find(NumType num) const {
  iterator r = std::lower_bound(this->begin(), this->end(), num);
  if (*r == num) return r;
  return this->end();
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator>::~BoundedSet() {
  deallocate();
}

template <class NumType, class Allocator>
BoundedSet<NumType, Allocator> &BoundedSet<NumType, Allocator>::operator=(
    const std::unordered_set<NumType> &other) {
  assert(!isImmutable);
  assert(other.setSize <= this->maxSize);

  this->setSize = 0;
  this->signature = 0;
  for (auto elem : other)
    this->insertRaw(elem, true);

  return *this;
}

template <class NumType, class Allocator>
bool BoundedSet<NumType, Allocator>::operator==(
    const BoundedSet<NumType, Allocator> &other) const {
  // Maximum size is not taken into account (that is correct).
  if (this->setSize != other.setSize)
    return false;

  if constexpr (CheckSignature) {
    if (this->signature != other.signature)
      return false;
  }

  for (SizeType i = 0; i < this->setSize; ++i) {
    if (this->setPtr[i] != other.setPtr[i])
      return false;
  }

  return true;
}

} // namespace eda::util
