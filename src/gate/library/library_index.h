//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <functional>
#include <vector>

namespace eda::gate::library {

/**
 * \brief Indexes for SCLibrary.
 * Will be invalidated if library is modified after creation.
 */
template <typename T>
class SCLibraryIndex {
public:
  using SelectionType = std::vector<T>;
private:
  using StorageType = std::vector<T>;
public:
  template<typename F>
  SCLibraryIndex(F && selectFunc): index{std::invoke(selectFunc)} {
    static_assert(std::is_same_v<std::invoke_result_t<F>, StorageType>);
  }

  typename StorageType::const_iterator begin() { return index.begin(); }
  typename StorageType::const_iterator end() { return index.end(); }
  typename StorageType::const_iterator cbegin() const { return index.cbegin(); }
  typename StorageType::const_iterator cend() const { return index.cend(); }

private:
  StorageType index;
};

} // namespace eda::gate::library