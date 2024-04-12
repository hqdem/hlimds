//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "util/singleton.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <cstdio>
#include <filesystem>
#include <memory.h>
#include <string>

namespace eda::gate::techmapper {

class LibraryManager : public util::Singleton<LibraryManager> {
public:
  bool loadLibrary(const std::string& filename) {
    TokenParser tokParser;
    file = fopen(filename.c_str(), "rb");
    ast = tokParser.parseLibrary(file,
                                 filename.c_str());

    AstParser parser(library, tokParser);
    parser.run(*ast);
    fclose(file);
    isLoaded = true;
    return true;
  }

  Library &getLibrary() {
    if (!isLoaded) {
      throw std::runtime_error("Library not loaded.");
    }
    return library;
  }

private:
  FILE *file;
  Group *ast;
  Library library;
  TokenParser tokParser;
  bool isLoaded = false;

  LibraryManager() : Singleton() {}
  friend class Singleton<LibraryManager>;
};

} // namespace eda::gate::techmapper
