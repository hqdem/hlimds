//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/array.h"
#include "gate/model/link.h"
#include "gate/model/object.h"
#include "gate/model/storage.h"
#include "gate/model/string.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace eda::gate::model {

//===----------------------------------------------------------------------===//
// Cell Type Attributes
//===----------------------------------------------------------------------===//

struct PhysicalProperties final {
  float area{0.};
  float delay{0.};
  float power{0.};

  uint32_t padding__{0};
};

static_assert(sizeof(PhysicalProperties) == 16);

struct Port final {
  using Width = uint32_t;
  using Flags = uint16_t;
  using Index = uint16_t;

  Port(const std::string &name, Width width, bool input, Index index = 0):
      nameID(allocateObject<String>(name)),
      width(width), input(input), index(index) {}

  Port(Width width, bool input, Index index = 0):
      width(width), input(input), index(index) {}

  Port(const std::string &name, bool input, Index index = 0):
      Port(name, 1, input, index) {}

  Port(bool input, Index index = 0):
      Port(1, input, index) {}

  Port(): Port(0, false, 0) {}

  bool hasName() const { return nameID != OBJ_NULL_ID; }
  std::string getName() const { return String::get(nameID); }

  StringID nameID{OBJ_NULL_ID};

  Width width;
  Flags input;
  Index index;
};

static_assert(sizeof(Port) == 16);

class CellTypeAttr final : public Object<CellTypeAttr, CellTypeAttrID> {
  friend class Storage<CellTypeAttr>;

public:
  using PortIndex = Port::Index;
  using PortWidth = Port::Width;

  using PinIndex = LinkEnd::PortType;

  using PortWidths = std::vector<PortWidth>;
  using PortVector = std::vector<Port>;

  static constexpr PortIndex Unknown = std::numeric_limits<PortIndex>::max();
  static constexpr PortWidth MaxBitWidth = std::numeric_limits<PortWidth>::max();
  static constexpr PortIndex MaxPortNum = std::numeric_limits<PortIndex>::max();

  static PortWidth checkBitWidth(const size_t w) {
    assert(w <= MaxBitWidth);
    return w;
  }

  static PortIndex checkPortNum(const size_t n) {
    assert(n <= MaxPortNum);
    return n;
  }

  static PortWidth getBitWidth(const PortWidths &widths) {
    size_t w{0};
    for (const auto width : widths) {
      w += width;
    }
    return checkBitWidth(w);
  }

  static PortIndex getInNum(const PortVector &ports) {
    size_t n{0};
    for (const auto &port : ports) {
      if (port.input) n++;
    }
    return checkPortNum(n);
  }

  static PortIndex getOutNum(const PortVector &ports) {
    size_t n{0};
    for (const auto &port : ports) {
      if (!port.input) n++;
    }
    return checkPortNum(n);
  }

  static PortWidth getInBitWidth(const PortVector &ports) {
    size_t n{0};
    for (const auto &port : ports) {
      if (port.input) n += port.width;
    }
    return checkBitWidth(n);
  }

  static PortWidth getOutBitWidth(const PortVector &ports) {
    size_t w{0};
    for (const auto &port : ports) {
      if (!port.input) w += port.width;
    }
    return checkBitWidth(w);
  }

  bool hasPortInfo() const {
    return nInPort != Unknown && nOutPort != Unknown;
  }

  PortIndex getInPortNum() const {
    assert(hasPortInfo());
    return nInPort;
  }

  PortIndex getOutPortNum() const {
    assert(hasPortInfo());
    return nOutPort;
  }

  const Port &getPort(const PortIndex i) const {
    assert(hasPortInfo());
    return ports[i];
  }

  const Port &getInPort(const PortIndex i) const {
    return getPort(i);
  }

  const Port &getOutPort(const PortIndex i) const {
    return getPort(nInPort + i);
  }

  PortWidth getWidth(const PortIndex i) const {
    return getPort(i).width;
  }

  PortWidth getInWidth(const PortIndex i) const {
    return getInPort(i).width;
  }

  PortWidth getInWidth() const {
    size_t w{0};
    for (PortIndex i = 0; i < nInPort; ++i) {
      w += getInWidth(i);
    }
    return checkBitWidth(w);
  }

  PortWidth getOutWidth(const PortIndex i) const {
    return getOutPort(i).width;
  }

  PortWidth getOutWidth() const {
    size_t w{0};
    for (PortIndex i = 0; i < nOutPort; ++i) {
      w += getOutWidth(i);
    }
    return w;
  }

  std::pair<PortIndex, PortWidth> mapPinToPort(const PinIndex i) const;

  PortVector getOrderedPorts() const {
    const PortIndex n = nInPort + nOutPort;
    PortVector ordered(n);
    for (PortIndex i = 0; i < n; ++i) {
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
  CellTypeAttr();
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
  const Array<Port> ports;

  /// Number of input (multi-bit) ports.
  uint16_t nInPort{Unknown};
  /// Number of output (multi-bit) ports.
  uint16_t nOutPort{Unknown};

  uint8_t padding__[24];
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
