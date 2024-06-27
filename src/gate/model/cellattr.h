//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/object.h"
#include "gate/model/storage.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
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
  using PortWidths = std::vector<uint16_t>;

  static constexpr uint16_t Unknown = 0xffff;
  static constexpr uint16_t MaxPorts = 256;

  static uint16_t getBitWidth(const PortWidths &widths) {
    size_t n = 0;
    for (const auto width : widths) {
      n += width;
    }
    assert(n < 0xffff);
    return n;
  }

  uint16_t getInWidth(uint16_t i) const {
    return width[i];
  }

  uint16_t getInWidth() const {
    uint16_t n = 0;
    for (uint16_t i = 0; i < nInPort; ++i) {
      n += getInWidth(i);
    }
    return n;
  }

  uint16_t getOutWidth(uint16_t i) const {
    return width[nInPort + i];
  }

  uint16_t getOutWidth() const {
    uint16_t n = 0;
    for (uint16_t i = 0; i < nOutPort; ++i) {
      n += getOutWidth(i);
    }
    return n;
  }

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

  CellTypeAttr(const PortWidths &widthIn, const PortWidths &widthOut):
      nInPort(widthIn.size()), nOutPort(widthOut.size()) {
    assert(widthIn.size() + widthOut.size() <= MaxPorts);
    size_t i = 0, n = 0;
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
  return allocateObject<CellTypeAttr>();
}

inline CellTypeAttrID makeCellTypeAttr(const CellTypeAttr::PortWidths &widthIn,
                                       const CellTypeAttr::PortWidths &widthOut) {
  return allocateObject<CellTypeAttr>(widthIn, widthOut);
}

inline CellTypeAttrID makeCellTypeAttr(uint16_t widthLhs,
                                       uint16_t widthRhs,
                                       uint16_t widthRes) {
  CellTypeAttr::PortWidths widthIn{widthLhs, widthRhs};
  CellTypeAttr::PortWidths widthOut{widthRes};

  return makeCellTypeAttr(widthIn, widthOut);
}

} // namespace eda::gate::model
