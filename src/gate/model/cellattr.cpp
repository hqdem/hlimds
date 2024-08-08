//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/celltype.h"

namespace eda::gate::model {

CellTypeAttr::CellTypeAttr(const PortVector &io):
    nInPort(getInNum(io)), nOutPort(getOutNum(io)) {
  assert(io.size() <= MaxPortNum);

  uint16_t index{0};
  uint32_t width{0};

  uint16_t i{0}, j{nInPort};
  for (const auto &port : io) {
    assert(port.width);
    uint16_t &k = port.input ? i : j;

    ports[k] = port;
    ports[k++].index = index;

    index += 1;
    width += port.width;
  }

  assert(width <= MaxBitWidth);
}

CellTypeAttr::CellTypeAttr(const PortWidths &widthIn,
                           const PortWidths &widthOut):
    nInPort(widthIn.size()), nOutPort(widthOut.size()) {
  assert(widthIn.size() + widthOut.size() <= MaxPortNum);

  uint16_t index{0};
  uint32_t width{0};

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

  assert(width <= MaxBitWidth);
}

} // namespace eda::gate::model
