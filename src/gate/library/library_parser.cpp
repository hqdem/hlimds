//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library.h"
#include "gate/library/library_characteristics.h"
#include "gate/library/library_parser.h"

namespace eda::gate::library {

void LibraryParser::loadLibrary(const path &filename) {
  if (filename == this->filename) return;

  this->filename = filename;
  file = fopen(filename.c_str(), "rb");
  ast = tokParser.parseLibrary(file,
                               filename.c_str());

  AstParser parser(library, tokParser);
  parser.run(*ast);
  fclose(file);
  isLoaded = true;

  SCLibrary::get().loadCells();
}

bool LibraryParser::isInit() {
  return isLoaded;
}

Library &LibraryParser::getLibrary() {
  return library;
}
} // namespace eda::gate::library