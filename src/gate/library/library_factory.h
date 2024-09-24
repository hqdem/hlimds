//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/library/cell_srcfile_parser_iface.h"
#include "gate/library/library.h"

namespace eda::gate::library {

/**
 * \brief Factory pattern for SCLibrary.
 * Also includes fillLibrary methods
 */
class SCLibraryFactory {
public:
  static SCLibrary newLibrary();
  static SCLibrary newLibrary(CellSourceFileParserIface& parser);
  static std::unique_ptr<SCLibrary> newLibraryUPtr();
  static std::unique_ptr<SCLibrary> newLibraryUPtr(CellSourceFileParserIface& parser);
  static bool fillLibrary(SCLibrary& library,
                          CellSourceFileParserIface& parser);
};

} // namespace eda::gate::library