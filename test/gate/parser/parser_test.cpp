//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include <unordered_map>

#include <lorina/diagnostics.hpp>
#include <lorina/verilog.hpp>

#include "gate/parser/reader_gate.h"

#include "gtest/gtest.h"

using namespace lorina;

TEST(ParserVTest, c17) {
  std::string filename = "test/data/gate/parser/my.v";
  text_diagnostics consumer;
  diagnostic_engine diag( &consumer );

  ReaderGate reader( "c17" );
  std::ofstream out ("test/data/gate/parser/my.dot");

  return_code result = read_verilog( filename, reader, &diag );
  reader.print();
  //  reader.dot(out);

  EXPECT_EQ(result, return_code::success);
}

