//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/gate_verilog.h"
#include "gate/premapper/mapper/mapper_test.h"
#include "gate/printer/dot.h"

#include "gtest/gtest.h"
#include <lorina/diagnostics.hpp>
#include <lorina/verilog.hpp>

#include <filesystem>
#include <unordered_map>

using namespace eda::gate::model;
using namespace eda::gate::parser::verilog;
using namespace eda::gate::premapper;
using namespace lorina;

using GateBinding = std::unordered_map<Gate::Link, Gate::Link>;
using GateIdMap = std::unordered_map<Gate::Id, Gate::Id>;
using Link = Gate::Link;

const std::filesystem::path subCatalog = "test/data/gate/parser/verilog";
const std::filesystem::path homePath = std::string(getenv("UTOPIA_HOME"));
const std::filesystem::path prefixPath = homePath / subCatalog;
const std::filesystem::path prefixPathIn = prefixPath;

TEST(AigPremapperVerilogTest, c17) {
  EXPECT_TRUE(parseFile("c17.v", PreBasis::AIG, prefixPathIn));
}

TEST(AigPremapperVerilogTest, c432) {
  EXPECT_TRUE(parseFile("c432.v", PreBasis::AIG, prefixPathIn));
}
