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

typedef uint64_t ObjectPage;
typedef char *SystemPage;

static constexpr uint64_t PAGE_SIZE = 64*1024*1024;
static constexpr uint64_t PAGE_MASK = PAGE_SIZE - 1;

class PageManager final : public util::Singleton<PageManager> {
  friend class util::Singleton<PageManager>;

public:
  /// Returns the object page.
  static constexpr uint64_t getPage(uint64_t objectID) {
    return objectID & ~PAGE_MASK;
  }

  /// Returns the object offset.
  static constexpr uint64_t getOffset(uint64_t objectID) {
    return objectID & PAGE_MASK;
  }

  /// Returns the object identifier.
  static constexpr uint64_t getObjectID(uint64_t page, uint64_t offset) {
    return page + offset;
  }

  /// Returns the object pointer.
  static constexpr void *getObjectPtr(SystemPage page, uint64_t offset) {
    return page + offset;
  }

  // TODO: Dummy address translator.
  SystemPage translate(ObjectPage objectPage) const {
    auto i = pageTable.find(objectPage);
    assert(i != pageTable.end());

    return i->second;
  }

  // TODO: Dummy page manager.
  std::pair<ObjectPage, SystemPage> allocate() {
    const SystemPage systemPage = allocatePage();
    const std::pair<ObjectPage, SystemPage> translation{objectPage, systemPage};

    const auto info = pageTable.insert(translation);
    assert(info.second);

    objectPage += PAGE_SIZE;
    return translation;
  }

private:
  PageManager() {}

  SystemPage allocatePage() {
    void *page = aligned_alloc(PAGE_SIZE, PAGE_SIZE);
    assert(page);

    // Advice Linux kernel to use huge pages.
    madvise(page, PAGE_SIZE, MADV_HUGEPAGE);
    return static_cast<SystemPage>(page);
  }

  /// Current object page.
  ObjectPage objectPage = 0;

  // TODO: Dummy page table.
  std::unordered_map<ObjectPage, SystemPage> pageTable;
};

} // namespace eda::gate::model
