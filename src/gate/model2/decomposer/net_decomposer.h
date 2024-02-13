//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/net.h"
#include "gate/model2/subnet.h"
#include "util/singleton.h"

#include <vector>

namespace eda::gate::model {

class NetDecomposer final : public util::Singleton<NetDecomposer> {
  friend class util::Singleton<NetDecomposer>;

public:
  std::vector<SubnetID> make(NetID netID) const;

private:
  NetDecomposer() {}
};

} // namespace eda::gate::model
