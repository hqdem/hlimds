//===----------------------------------------------------------------------===//
//
// part of the utopia eda project, under the apache license v2.0
// spdx-license-identifier: apache-2.0
// copyright 2021 isp ras (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model2/cell.h"
#include "gate/model2/link.h"

namespace eda::gate::model {

const Cell &LinkEnd::getCell() const {
  return Cell::get(getCellID());
}

} // namespace eda::gate::model
