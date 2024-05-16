//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/parser/graphml_test_utils.h"
#include "util/env.h"

using path = std::filesystem::path;

namespace eda::gate::parser::graphml {

SubnetBuilder parse(std::string fileName, ParserData *data) {

  const path dir = path(std::string(getenv("UTOPIA_HOME"))) /
      "test" /
      "data" /
      "gate" /
      "parser" /
      "graphml" /
      "OpenABC" /
      "graphml_openabcd";

  const path home = eda::env::getHomePath();
  const path file = home / dir / fileName;

  uassert(std::filesystem::exists(file.string()),
                                  "File doesn't exist" << std::endl);

  GraphMlParser parser;
  return
      data ? parser.parse(file.string()) : parser.parse(file.string(), *data);
}

} // namespace eda::gate::parser::graphml
