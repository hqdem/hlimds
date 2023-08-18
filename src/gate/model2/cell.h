//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/celltype.h"
#include "gate/model2/link.h"
#include "gate/model2/object.h"

namespace eda::gate::model {

class Cell final {
public:
  using ID = CellID;

  Cell(CellTypeID typeID): typeSID(typeID.getSID()) {}

  bool isIn()    const { return typeSID == CELL_TYPE_SID_IN;    }
  bool isOut()   const { return typeSID == CELL_TYPE_SID_OUT;   }
  bool isZero()  const { return typeSID == CELL_TYPE_SID_ZERO;  }
  bool isOne()   const { return typeSID == CELL_TYPE_SID_ONE;   }
  bool isNop()   const { return typeSID == CELL_TYPE_SID_NOP;   }
  bool isNot()   const { return typeSID == CELL_TYPE_SID_NOT;   }
  bool isAnd()   const { return typeSID == CELL_TYPE_SID_AND;   }
  bool isOr()    const { return typeSID == CELL_TYPE_SID_OR;    }
  bool isXor()   const { return typeSID == CELL_TYPE_SID_XOR;   }
  bool isNand()  const { return typeSID == CELL_TYPE_SID_NAND;  }
  bool isNor()   const { return typeSID == CELL_TYPE_SID_NOR;   }
  bool isXnor()  const { return typeSID == CELL_TYPE_SID_XNOR;  }
  bool isMaj()   const { return typeSID == CELL_TYPE_SID_MAJ;   }
  bool isLatch() const { return typeSID == CELL_TYPE_SID_LATCH; }
  bool isDff()   const { return typeSID == CELL_TYPE_SID_DFF;   }
  bool isDffRs() const { return typeSID == CELL_TYPE_SID_DFFrs; }

  uint16_t getFanin()  const { return fanin;  }
  uint16_t getFanout() const { return fanout; }

  void setFanin(uint16_t value)  { fanin  = value; }
  void setFanout(uint16_t value) { fanout = value; }

private:
  /// Cell type short identifier.
  const uint32_t typeSID;

  uint16_t fanin;
  uint16_t fanout;

  Link link[3];
};

static_assert(sizeof(Cell) == CellID::Size);

} // namespace eda::gate::model
