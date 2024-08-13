//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "diag/diagnostics.h"

namespace eda::diag {

void Diagnostics::initialize() {
  diagnosis.reset();
  groups = std::stack<Entry *>{};
  groups.push(&diagnosis);
}

void Diagnostics::beginGroup(const std::string &msg) {
  auto *scope = groups.top();
  scope->more.emplace_back(msg);
  groups.push(&scope->more.back());
}

void Diagnostics::endGroup() {
  assert(groups.size() > 1);
  auto *group = groups.top();
  groups.pop();
  auto *scope = groups.top();
  if (group->more.empty()) {
    scope->more.resize(scope->more.size() - 1);
  }
}

void Diagnostics::add(const Entry &entry) {
  assert(!groups.empty());

  if (entry.lvl == BEGIN) {
    beginGroup(entry.msg);
  } else if (entry.lvl == END) {
    endGroup();
  } else {
    if (entry.lvl == WARN)
      nWarn++;
    else if (entry.lvl == ERROR)
      nError++;

    auto *scope = groups.top();
    scope->more.push_back(entry);
  }
}

} // namespace eda::diag
