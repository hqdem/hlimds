//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/cell_srcfile_parser_iface.h"
#include "gate/library/library.h"
#include "gate/library/library_factory.h"

namespace eda::gate::library {

SCLibrary SCLibraryFactory::newLibrary() {
  return {};
}
SCLibrary SCLibraryFactory::newLibrary(CellSourceFileParserIface& parser) {
  SCLibrary library;
  fillLibrary(library, parser);
  return library;
}

std::unique_ptr<SCLibrary> SCLibraryFactory::newLibraryUPtr() {
  return std::unique_ptr<SCLibrary>(new SCLibrary());
}

std::unique_ptr<SCLibrary> SCLibraryFactory::newLibraryUPtr(
    CellSourceFileParserIface& parser) {
  std::unique_ptr<SCLibrary> library(new SCLibrary());
  fillLibrary(*library, parser);
  return library;
}

bool SCLibraryFactory::fillLibrary(SCLibrary& library,
                                  CellSourceFileParserIface& parser) {
  library.addCells(parser.extractCells());
  library.addWLMs(parser.extractWLMs());
  library.addTemplates(parser.extractTemplates());
  auto properties = parser.extractProperties();
  library.addProperties(properties.defaultWLM,
                        std::move(properties.WLSelection));
  return true;
}

} // namespace eda::gate::library