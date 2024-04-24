//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include <iostream>

#define LOG_INFO  std::cerr << "(" << __FILE__ << ", " << __LINE__ << "): "
#define LOG_ERROR LOG_INFO
#define LOG_WARN  LOG_INFO
#define LOG_FATAL LOG_INFO

#ifdef UTOPIA_DEBUG
#define LOG_DEBUG(x) std::cerr << x << std::endl;
#else
#define LOG_DEBUG(x)
#endif // UTOPIA_DEBUG

#define LOG(severity) LOG_##severity

#define CHECK(x) \
  if (!(x)) \
    LOG(FATAL) << "Check failed: " #x << ": "
