//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/object.h"

#include <cassert>
#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Cell Type Attributes
//===----------------------------------------------------------------------===//

struct PhysicalProperties {
  float area{0.0};
  float delay{0.0};
  float power{0.0};
};

class CellTypeAttr final : public Object<CellTypeAttr, CellTypeAttrID> {
  friend class Storage<CellTypeAttr>;

public:
  static constexpr uint16_t Unknown = 0xffff;
  static constexpr uint16_t MaxPorts = 256;

  /// Number of input (multi-bit) ports.
  uint16_t nInPort{Unknown};
  /// Number of output (multi-bit) ports.
  uint16_t nOutPort{Unknown};

  /// Input/output port widths (inputs come before outputs).
  uint16_t width[MaxPorts];

  /// Generalized physical characteristics.
  PhysicalProperties props;

private:
  CellTypeAttr() {}

  CellTypeAttr(const std::vector<uint16_t> &widthIn,
               const std::vector<uint16_t> &widthOut):
      nInPort(widthIn.size()), nOutPort(widthOut.size()) {
    assert(widthIn.size() + widthOut.size() <= MaxPorts);
    size_t i = 0;
    size_t n = 0;
    for (const auto w : widthIn) {
      width[i++] = w;
      n += w;
    }
    for (const auto w : widthOut) {
      width[i++] = w;
      n += w;
    }
    assert(n < 0xffff);
  }

  /// TODO: To be updated.
  uint16_t reserved[247];
};

static_assert(sizeof(CellTypeAttr) == CellTypeAttrID::Size);

//===----------------------------------------------------------------------===//
// Cell Type Attributes Builder
//===----------------------------------------------------------------------===//

inline CellTypeAttrID makeCellTypeAttr() {
  return allocate<CellTypeAttr>();
}

inline CellTypeAttrID makeCellTypeAttr(const std::vector<uint16_t> &widthIn,
                                       const std::vector<uint16_t> &widthOut) {
  return allocate<CellTypeAttr>(widthIn, widthOut);
}

inline CellTypeAttrID makeCellTypeAttr(uint16_t widthLhs, uint16_t widthRhs,
                                       uint16_t widthRes) {
  std::vector<uint16_t> widthIn{widthLhs, widthRhs};
  std::vector<uint16_t> widthOut{widthRes};

  return makeCellTypeAttr(widthIn, widthOut);
}

} // namespace eda::gate::model
