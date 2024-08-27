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
#include "gate/model/string.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Cell Type Attributes
//===----------------------------------------------------------------------===//

struct PhysicalProperties final {
  float area{0.};
  float delay{0.};
  float power{0.};
  uint32_t __padding{0};
};
static_assert(sizeof(PhysicalProperties) == 16);

struct Port final {
  Port(const std::string &name, uint16_t width, bool input, uint16_t index = 0):
      nameID(allocateObject<String>(name)),
      width(width), input(input), index(index) {}

  Port(uint16_t width, bool input, uint16_t index = 0):
      nameID(OBJ_NULL_ID),
      width(width), input(input), index(index) {}

  Port(const std::string &name, bool input, uint16_t index = 0):
      Port(name, 1, input, index) {}

  Port(bool input, uint16_t index = 0):
      Port(1, input, index) {}

  Port(): Port(0, false, 0) {}

  std::string getName() const { return String::get(nameID); }

  StringID nameID;
  uint16_t width;
  uint16_t input;
  uint16_t index;
  uint16_t __padding{0};
};
static_assert(sizeof(Port) == 16);

class CellTypeAttr final : public Object<CellTypeAttr, CellTypeAttrID> {
  friend class Storage<CellTypeAttr>;

public:
  using PortWidths = std::vector<uint16_t>;
  using PortVector = std::vector<Port>;

  static constexpr uint16_t Unknown = 0xffff;
  static constexpr uint16_t MaxBitWidth = 0xfffe;
  static constexpr uint16_t MaxPortNum = 254;

  static uint16_t checkBitWidth(size_t width) {
    assert(width <= MaxBitWidth);
    return width;
  }

  static uint16_t getBitWidth(const PortWidths &widths) {
    size_t n{0};
    for (const auto width : widths) {
      n += width;
    }
    return checkBitWidth(n);
  }

  static uint16_t getInNum(const PortVector &ports) {
    size_t n{0};
    for (const auto &port : ports) {
      if (port.input) n++;
    }
    return n;
  }

  static uint16_t getOutNum(const PortVector &ports) {
    size_t n{0};
    for (const auto &port : ports) {
      if (!port.input) n++;
    }
    return n;
  }

  static uint16_t getInBitWidth(const PortVector &ports) {
    size_t n{0};
    for (const auto &port : ports) {
      if (port.input) n += port.width;
    }
    return checkBitWidth(n);
  }

  static uint16_t getOutBitWidth(const PortVector &ports) {
    size_t n{0};
    for (const auto &port : ports) {
      if (!port.input) n += port.width;
    }
    return checkBitWidth(n);
  }

  bool hasPortInfo() const {
    return nInPort != Unknown && nOutPort != Unknown;
  }

  uint16_t getInPortNum() const {
    assert(hasPortInfo());
    return nInPort;
  }

  uint16_t getOutPortNum() const {
    assert(hasPortInfo());
    return nOutPort;
  }

  const Port &getPort(uint16_t i) const {
    assert(hasPortInfo());
    return ports[i];
  }

  const Port &getInPort(uint16_t i) const {
    return getPort(i);
  }

  const Port &getOutPort(uint16_t i) const {
    return getPort(nInPort + i);
  }

  uint16_t getWidth(uint16_t i) const {
    return getPort(i).width;
  }

  uint16_t getInWidth(uint16_t i) const {
    return getInPort(i).width;
  }

  uint16_t getInWidth() const {
    uint16_t n{0};
    for (uint16_t i = 0; i < nInPort; ++i) {
      n += getInWidth(i);
    }
    return n;
  }

  uint16_t getOutWidth(uint16_t i) const {
    return getOutPort(i).width;
  }

  uint16_t getOutWidth() const {
    uint16_t n{0};
    for (uint16_t i = 0; i < nOutPort; ++i) {
      n += getOutWidth(i);
    }
    return n;
  }

  std::pair<uint16_t, uint16_t> mapPinToPort(uint16_t i) const;

  PortVector getOrderedPorts() const {
    const size_t n = nInPort + nOutPort;
    PortVector ordered(n);
    for (size_t i = 0; i < n; ++i) {
      const auto &port = ports[i];
      ordered[port.index] = port;
    }
    return ordered;
  }

  const PhysicalProperties &getPhysProps() const {
    return props;
  }

  void setPhysProps(const PhysicalProperties &props) {
    this->props = props;
  }

private:
  CellTypeAttr() {}
  CellTypeAttr(const PortVector &io);
  CellTypeAttr(const PortVector &io,
               const PhysicalProperties &props);
  CellTypeAttr(const PortWidths &widthIn,
               const PortWidths &widthOut);
  CellTypeAttr(const PortWidths &widthIn,
               const PortWidths &widthOut,
               const PhysicalProperties &props);

  /// Generalized physical characteristics.
  PhysicalProperties props;

  /// Input/output ports (inputs come before outputs).
  Port ports[MaxPortNum];

  /// Number of input (multi-bit) ports.
  uint16_t nInPort{Unknown};
  /// Number of output (multi-bit) ports.
  uint16_t nOutPort{Unknown};

  uint8_t __padding[12];
};

static_assert(sizeof(CellTypeAttr) == CellTypeAttrID::Size);

//===----------------------------------------------------------------------===//
// Cell Type Attributes Builder
//===----------------------------------------------------------------------===//

inline CellTypeAttrID makeCellTypeAttr() {
  return allocateObject<CellTypeAttr>();
}

inline CellTypeAttrID makeCellTypeAttr(const CellTypeAttr::PortVector &ports) {
  return allocateObject<CellTypeAttr>(ports);
}

inline CellTypeAttrID makeCellTypeAttr(const CellTypeAttr::PortVector &ports,
                                       const PhysicalProperties &props) {
  return allocateObject<CellTypeAttr>(ports, props);
}

inline CellTypeAttrID makeCellTypeAttr(const CellTypeAttr::PortWidths &widthIn,
                                       const CellTypeAttr::PortWidths &widthOut) {
  return allocateObject<CellTypeAttr>(widthIn, widthOut);
}

inline CellTypeAttrID makeCellTypeAttr(const CellTypeAttr::PortWidths &widthIn,
                                       const CellTypeAttr::PortWidths &widthOut,
                                       const PhysicalProperties &props) {
  return allocateObject<CellTypeAttr>(widthIn, widthOut, props);
}

inline CellTypeAttrID makeCellTypeAttr(uint16_t widthLhs,
                                       uint16_t widthRhs,
                                       uint16_t widthRes) {
  CellTypeAttr::PortWidths widthIn{widthLhs, widthRhs};
  CellTypeAttr::PortWidths widthOut{widthRes};

  return makeCellTypeAttr(widthIn, widthOut);
}

} // namespace eda::gate::model
