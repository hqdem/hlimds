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
#include <stack>
#include <string>
#include <vector>

namespace eda::diag {

enum Severity : uint8_t {
  /// Information.
  NOTE,
  /// Warning.
  WARN,
  /// Error.
  ERROR,
  /// Beginning of a group.
  BEGIN,
  /// End of a group.
  END,
  /// Group of messages.
  GROUP = BEGIN
};

/// Represents the severity level as a string.
constexpr const char *getSeverityText(const uint8_t lvl) {
  switch (lvl) {
  case NOTE:
    return "NOTE";
  case WARN:
    return "WARNING";
  case ERROR:
    return "ERROR";
  case BEGIN:
    return "";
  case END:
    return "";
  default:
    return "UNKNOWN";
  }
}

inline std::string getSeverityString(const uint8_t lvl) {
  return std::string(getSeverityText(lvl));
}

struct Entry final {
  Entry(const Severity lvl, const std::string &msg): lvl(lvl), msg(msg) {}
  explicit Entry(const std::string &msg = ""): lvl(GROUP), msg(msg) {}
  
  Entry(const Entry &other) = default;
  Entry &operator =(const Entry &other) = default;

  void reset() { this->~Entry(); new (this) Entry(); }

  uint8_t lvl{GROUP};
  std::string msg;

  std::vector<Entry> more;
};

class Diagnostics final {
public:
  static Diagnostics &getDefault() {
    static Diagnostics diagnostics;
    return diagnostics;
  }

  Diagnostics() { initialize(); }

  const Entry &getDiagnosis() const { return diagnosis; }

  unsigned getWarnNum()  const { return nWarn;  }
  unsigned getErrorNum() const { return nError; }

  void initialize();

  void beginGroup(const std::string &msg);

  void endGroup();

  void add(const Entry &entry);

private:
  unsigned nWarn{0};
  unsigned nError{0};

  Entry diagnosis;
  std::stack<Entry *> groups;
};

} // namespace eda::diag
