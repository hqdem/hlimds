//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/celltype.h"

namespace eda::gate::model {

static CellTypeAttr::PortVector convert(
    const CellTypeAttr::PortVector &io) {
  assert(io.size() <= CellTypeAttr::MaxPortNum);
  CellTypeAttr::PortVector ports(io.size());

  Port::Index index{0};
  Port::Width width{0};

  Port::Index i{0}, j{CellTypeAttr::getInNum(io)};
  for (const auto &port : io) {
    assert(port.width);
    Port::Index &k = port.input ? i : j;

    ports[k] = port;
    ports[k++].index = index;

    index += 1;
    width += port.width;
  }

  assert(width <= CellTypeAttr::MaxBitWidth);
  return ports;
}

static CellTypeAttr::PortVector convert(
    const CellTypeAttr::PortWidths &widthIn,
    const CellTypeAttr::PortWidths &widthOut) {
  const auto size = widthIn.size() + widthOut.size();
  assert(size <= CellTypeAttr::MaxPortNum);
  CellTypeAttr::PortVector ports(size);

  Port::Index index{0};
  Port::Width width{0};

  for (const auto w : widthIn) {
    assert(w);
    ports[index] = Port{w, true /* input */, index};
    index += 1;
    width += w;
  }

  for (const auto w : widthOut) {
    assert(w);
    ports[index] = Port{w, false /* output */, index};
    index += 1;
    width += w;
  }

  assert(width <= CellTypeAttr::MaxBitWidth);
  return ports;
}

CellTypeAttr::CellTypeAttr():
    ports(ArrayBlock<Port>::allocate(0, true, true)) {}

CellTypeAttr::CellTypeAttr(const PortVector &io):
    ports(ArrayBlock<Port>::allocate(convert(io), true, true)),
    nInPort(getInNum(io)), nOutPort(getOutNum(io)) {}

CellTypeAttr::CellTypeAttr(const PortVector &io,
                           const PhysicalProperties &props):
    CellTypeAttr(io) {
  setPhysProps(props);
}

CellTypeAttr::CellTypeAttr(const PortWidths &widthIn,
                           const PortWidths &widthOut):
    ports(ArrayBlock<Port>::allocate(convert(widthIn, widthOut), true, true)),
    nInPort(widthIn.size()), nOutPort(widthOut.size()) {}

CellTypeAttr::CellTypeAttr(const PortWidths &widthIn,
                           const PortWidths &widthOut,
                           const PhysicalProperties &props):
    CellTypeAttr(widthIn, widthOut) {
  setPhysProps(props);
}

std::pair<CellTypeAttr::PortIndex, CellTypeAttr::PortWidth>
CellTypeAttr::mapPinToPort(const PinIndex i) const {
  assert(hasPortInfo());

  PortWidth width{0};
  for (PortIndex index = 0; index < nInPort + nOutPort; ++index) {
    if (i < width + ports[index].width)
      return {index, i - width};
    width += ports[index].width;
  }

  assert(false && "Pin is out of range");
  return {Unknown, 0};
}

} // namespace eda::gate::model
