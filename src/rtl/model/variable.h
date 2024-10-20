//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <fmt/format.h>

#include <cstddef>
#include <iostream>
#include <vector>

namespace eda::rtl::model {

/**
 * \brief Represents a data type.
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Type final {
public:
  enum Kind {
    /// Signed integer (width).
    SINT,
    /// Unsigned integer (width).
    UINT,
    /// Floating-point number (width, fract).
    FLOAT
  };

  Type(Kind kind, std::size_t width, std::size_t fract = 0):
    _kind(kind), _width(width), _fract(fract) {}

  Kind kind() const { return _kind; }
  std::size_t width() const { return _width; }
  std::size_t fract() const { return _fract; }

private:
  const Kind _kind;
  const std::size_t _width;
  const std::size_t _fract;
};

/**
 * \brief Represents a variable (wire or register).
 * \author <a href="mailto:kamkin@ispras.ru">Alexander Kamkin</a>
 */
class Variable final {
public:
  enum Kind {
    WIRE,
    REG
  };

  enum Bind {
    INPUT,
    OUTPUT,
    INNER
  };

  Variable(const std::string &name, Kind kind, Bind bind, const Type &type):
    _name(name), _kind(kind), _bind(bind), _type(type) {}

  Variable(const std::string &name, Kind kind, const Type &type):
    _name(name), _kind(kind), _bind(INNER), _type(type) {}
 
  Variable(const Variable &rhs) = default;
  Variable(Variable &&rhs) = default;

  const std::string &name() const { return _name; }
  Kind kind() const { return _kind; }
  Bind bind() const { return _bind; }
  const Type &type() const { return _type; }

private:
  const std::string _name;
  const Kind _kind;
  const Bind _bind;
  const Type _type;
};

inline std::string unique_name(const std::string &prefix) {
  static int i = 0;
  return fmt::format("{}_{}", prefix.c_str(), i++);
}

inline std::ostream &operator <<(std::ostream &out, const Variable &variable) {
  return out << variable.name();
}

} // namespace eda::rtl::model
