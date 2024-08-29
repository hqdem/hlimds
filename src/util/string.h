//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>

namespace eda::util {

inline bool starts_with(const std::string &string, const std::string &prefix) {
  return string.size() >= prefix.size()
      && string.compare(0, prefix.size(), prefix) == 0;
}

inline bool ends_with(const std::string &string, const std::string &suffix) {
  return string.size() >= suffix.size()
      && string.compare(string.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

template<typename Predicate>
inline std::string getSubString(const std::string &source, size_t lhs,
                                size_t rhs, Predicate predicate) {
  std::string buf;
  std::copy_if(source.begin() + lhs, source.begin() + rhs,
               std::back_inserter(buf), predicate);
  return buf;
}

template<typename Predicate>
inline std::string getSubString(const std::string &source,
                                const std::string &lhs, const std::string &rhs,
                                Predicate predicate) {
  auto begin = source.find(lhs);
  auto end = source.find(rhs);
  return getSubString(source, begin, end, predicate);
}

} // namespace eda::util
