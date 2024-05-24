//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/subnet.h"
#include "kitty/kitty.hpp"
#include "util/serializer.h"

#include <iostream>
#include <sstream>

namespace eda::gate::model {

class SubnetSerializer : public util::Serializer<SubnetID> {

public:
  void serialize(std::ostream &out, const SubnetID &id) override;
  SubnetID deserialize(std::istream &in) override;
};

using SubnetListSerializer =
    util::VectorSerializer<SubnetID, SubnetSerializer>;

class TTSerializer : public util::Serializer<kitty::dynamic_truth_table> {

public:
  void serialize(std::ostream &out, const kitty::dynamic_truth_table &obj);
  kitty::dynamic_truth_table deserialize(std::istream &in);
};

} // namespace eda::gate::model
