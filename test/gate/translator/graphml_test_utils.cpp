//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/graphml_test_utils.h"
#include "util/env.h"

using path = std::filesystem::path;

namespace eda::gate::translator {

std::shared_ptr<Builder> translateGmlOpenabc(const std::string &fileName,
                                             ParserData *data) {

  const path dir = path(std::string(getenv("UTOPIA_HOME"))) /
      "test" /
      "data" /
      "gate" /
      "parser" /
      "graphml" /
      "OpenABC" /
      "graphml_openabcd";

  const path home = eda::env::getHomePath();
  const path file = home / dir / (fileName + ".bench.graphml");

  uassert(std::filesystem::exists(file.string()),
                                  "File doesn't exist" << std::endl);
  GmlTranslator translator;
  return data ?
      translator.translate(file.string(), *data) :
      translator.translate(file.string());
}

} // namespace eda::gate::translator
