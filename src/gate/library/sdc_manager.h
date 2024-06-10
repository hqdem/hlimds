//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "util/singleton.h"

#include <filesystem>

namespace eda::gate::library {

class SDC {
public: // TODO
  float area;
  float arrivalTime;
};

class SDCManager : public util::Singleton<SDCManager> {
  using path = std::filesystem::path;

public:
  bool loadSDC(const path &filename) {
    // TODO
    sdc.area = 100000000;
    sdc.arrivalTime = 10000000000;
    this->filename = filename;
    isLoaded = true;
    return true;
  }

  SDC &getSDC() {
    if (!isLoaded) {
      throw std::runtime_error("Library not loaded.");
    }
    return sdc;
  }

  const path &getSDCName() {
    return filename;
  }

  bool isInitialized() {
    return isLoaded;
  }

private:
  bool isLoaded = false;
  path filename;
  SDC sdc;

  SDCManager() : Singleton() {}
  friend class Singleton<SDCManager>;
};

} // namespace eda::gate::library
