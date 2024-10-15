//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/subnet.h"
#include "gate/translator/logdb.h"

#include <memory>

namespace eda::gate::translator {

static void parseLine(const std::shared_ptr<model::SubnetBuilder> &builderPtr,
                      model::Subnet::LinkList &links, 
                      const std::string &line,
                      const char delimiter) {

  std::istringstream stream(line);
  std::string buf;
  std::getline(stream, buf, delimiter);
  auto symbol = model::getSymbol(buf);

  model::Subnet::LinkList cellLinks;
  for (;std::getline(stream, buf, delimiter);) {
    model::Subnet::Link link;
    uint16_t start = 0;
    if (buf.front() == '~') {
      link.inv = 1;
      ++start;
    }
    auto pointPos = buf.find_first_of('.');
    auto idx = buf.substr(start, pointPos - start);
    link.idx = std::stoul(idx);
    if (pointPos != std::string::npos) {
      idx = buf.substr(++pointPos);
      link.out = std::stoul(idx);
    }
    cellLinks.push_back(link);
  }
  const auto link = builderPtr->addCell(symbol, cellLinks);
  links.push_back(link);
}

optimizer::NpnDatabase LogDbTranslator::translate(std::istream &in) const {
  const char delimiter = ' ';
  optimizer::NpnDatabase db;
  model::Subnet::LinkList links;
  auto builderPtr = std::make_shared<model::SubnetBuilder>();

  for (std::string line; std::getline(in, line);) {
    if (line.empty()) {
      db.push(builderPtr->make());
      builderPtr.reset(new model::SubnetBuilder());
      links.clear();
      continue;
    }
    parseLine(builderPtr, links, line, delimiter);
  }
  return db;
}

} // namespace eda::gate::translator
