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

namespace eda::gate::library {

class LibertyManager : public util::Singleton<LibertyManager> {
  using path = std::filesystem::path;

public:
  bool loadLibrary(const path &filename) {
    this->filename = filename;
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

  const std::string getLibraryName() {
    return filename;
  }

  bool isInitialized() {
    return isLoaded;
  }

private:
  FILE *file;
  Group *ast;
  Library library;
  TokenParser tokParser;
  bool isLoaded = false;
  path filename;

  LibertyManager() : Singleton() {}
  friend class Singleton<LibertyManager>;
};

} // namespace eda::gate::library
