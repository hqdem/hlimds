//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"

#include <string>

namespace eda::gate::model {

class String final {
public:
  using ID = StringID;

  String(const std::string &value): value(value) {}
  operator std::string() const { return value; }

  std::string value;
};

static_assert(sizeof(String) == StringID::Size);

} // namespace eda::gate::model
