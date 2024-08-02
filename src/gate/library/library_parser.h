//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "util/singleton.h"

// FIXME: fix includes!!!
#include <kitty/print.hpp>

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <filesystem>
#include <memory.h>
#include <regex>
#include <string>

namespace eda::gate::library {

class LibraryParser : public util::Singleton<LibraryParser> {
  friend class Singleton<LibraryParser>;

  using path = std::filesystem::path;

public:
  void loadLibrary(const path &filename);
  bool isInit();
  Library &getLibrary();

private:
  FILE *file;
  Group *ast;
  Library library;
  TokenParser tokParser;
  bool isLoaded = false;
  path filename;

  LibraryParser() : Singleton() {}
};

} // namespace eda::gate::library
