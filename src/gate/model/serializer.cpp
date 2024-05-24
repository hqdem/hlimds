//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/serializer.h"

#include <algorithm>
#include <bitset>

namespace eda::gate::model {

void SubnetSerializer::serialize(std::ostream &out, const SubnetID &id) {
  const Subnet &subnet = Subnet::get(id);
  uint32_t size = subnet.size();
  util::pushIntoStream(out, size);
  const Array<Subnet::Entry> &array = subnet.getEntries();
  for (uint32_t i = 0; i < size; i++) {
    util::pushIntoStream(out, array[i]);
  }
}

SubnetID SubnetSerializer::deserialize(std::istream &in) {
  uint32_t size;
  util::pullFromStream(in, size);

  SubnetBuilder sb;
  Subnet::Entry current;
  for (uint32_t i = 0; i < size; i++) {
    util::pullFromStream(in, current);
    const Subnet::Cell &cell = current.cell;
    Subnet::LinkList links(cell.link, cell.link + cell.arity);
    sb.addCell(cell.getTypeID(), links);
  }
  return sb.make();
}

void TTSerializer::serialize(std::ostream &out,
                             const kitty::dynamic_truth_table &obj) {
  uint32_t nv = obj.num_vars();
  util::pushIntoStream(out, nv);
  kitty::print_raw(obj, out);
}

kitty::dynamic_truth_table TTSerializer::deserialize(std::istream &in) {
  uint32_t nv;
  util::pullFromStream(in, nv);
  kitty::dynamic_truth_table tt(nv);
  kitty::create_from_raw(tt, in);
  return tt;
}

} // namespace eda::gate::model
