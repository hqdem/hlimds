//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/translator/graphml.h"

#include <filesystem>
#include <string>

using Builder       = eda::gate::translator::GmlTranslator::Builder;
using GmlTranslator = eda::gate::translator::GmlTranslator;
using ParserData    = GmlTranslator::ParserData;
using SubnetID      = eda::gate::model::SubnetID;

namespace eda::gate::translator {

std::shared_ptr<Builder> translateGmlOpenabc(const std::string &fileName,
                                             ParserData *data = nullptr);

} // namespace eda::gate::translator
