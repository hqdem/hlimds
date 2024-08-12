//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace eda::diag {

enum Severity : uint8_t {
  NOTE,
  WARN,
  ERROR
};

struct Entry final {
  Entry(const Severity lvl, const std::string &msg):
    lvl(lvl), msg(msg) {}

  Entry(const Entry &other) = default;
  Entry &operator =(const Entry &other) = default;

  std::string getLevel() const;

  uint8_t lvl;
  std::string msg;
};

class Diagnostics final {
public:
  void initialize() {
    entries.clear();
  }

  const std::vector<Entry> &get() const {
    return entries;
  }

  void add(const Entry &entry) {
    entries.push_back(entry);
  }

private:
  std::vector<Entry> entries;
};

} // namespace eda::diag
