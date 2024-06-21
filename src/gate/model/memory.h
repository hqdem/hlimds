//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "util/singleton.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>

#include <sys/mman.h>

namespace eda::gate::model {

typedef char *SystemPage;

constexpr uint64_t SMALL_PAGE_SIZE = 1*1024*1024;
constexpr uint64_t LARGE_PAGE_SIZE = 64*1024*1024;

class PageManager final : public util::Singleton<PageManager> {
  friend class util::Singleton<PageManager>;

public:
  /// Returns the object pointer.
  static constexpr void *getObjPtr(
      const SystemPage page, const uint64_t offset) {
    return page + offset;
  }

  SystemPage allocate(const uint64_t pageSize) {
    void *page = aligned_alloc(pageSize, pageSize);
    assert(page);

    // Advice Linux kernel to use huge pages.
    madvise(page, pageSize, MADV_HUGEPAGE);
    return static_cast<SystemPage>(page);
  }

private:
  PageManager() {}
};

} // namespace eda::gate::model
