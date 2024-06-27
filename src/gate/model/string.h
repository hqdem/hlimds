//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/object.h"
#include "gate/model/storage.h"

#include <string>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// String
//===----------------------------------------------------------------------===//

class String final : public Object<String, StringID> {
  friend class Storage<String>;

public:
  operator std::string() const { return value; }

private:
  String(const std::string &value): value(value) {}

  std::string value;
};

static_assert(sizeof(String) == StringID::Size);

//===----------------------------------------------------------------------===//
// String Builder
//===----------------------------------------------------------------------===//

inline StringID makeString(const std::string &value) {
  return allocateObject<String>(value);
}

} // namespace eda::gate::model
