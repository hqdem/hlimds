//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/npndb.h"

namespace eda::gate::optimizer {

using TT = NPNDatabase::TT;

NPNDatabase::ResultIterator NPNDatabase::get(const TT &tt) {
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  return ResultIterator(storage[utils::getTT(config)], utils::inverse(t));
}

NPNDatabase::ResultIterator NPNDatabase::get(const Subnet &subnet) {
  TT tt = model::evaluate(subnet)[0];
  return get(tt);
}

void NPNDatabase::printDot(std::ostream &out, const TT &tt,
                           const std::string &name, const bool quiet) {
  NPNDatabase::ResultIterator iterator = get(tt);
  Printer::getPrinter(Format::DOT)
      .print(out, Subnet::get(iterator.get()), name);
}

void NPNDatabase::printDotFile(const TT &tt, const std::string &fileName, 
                           const std::string &name, const bool quiet) {
  std::ofstream out;
  out.open(fileName);
  if (out.is_open()) {
    printDot(out, tt, name, quiet);
  }
  out.close();
}

void NPNDatabase::printInfo(std::ostream &out, const TT &tt, const bool quiet) {
  NPNDatabase::ResultIterator iterator = get(tt);
  Subnet &subnet1 = Subnet::get(iterator.get());
  printInfoSub(out, subnet1);
}

void NPNDatabase::printInfoSub(std::ostream &out, const Subnet &subnet) {
  out << "nIn: " << subnet.getInNum() << "\n";
  out << "nOut: " << subnet.getOutNum() << "\n";
  out << "nEntry: " << subnet.size() << "\n";
}

NPNDatabase::NPNTransformation NPNDatabase::push(const SubnetID &id) {
  TT tt = model::evaluate(Subnet::get(id))[0];
  auto config = kitty::exact_npn_canonization(tt);
  NPNTransformation t = utils::getTransformation(config);
  auto newId = utils::npnTransform(Subnet::get(id), t);
  storage[std::get<0>(config)].push_back(newId);
  return t;
}

void NPNDatabase::erase(const TT &tt) {
  storage.erase(tt);
}

NPNDatabase NPNDatabase::importFrom(const std::string &filename) {
  std::ifstream in(filename);
  NPNDatabase result = NPNDatabaseSerializer().deserialize(in);
  return result;
}

void NPNDatabase::exportTo(const std::string &filename) const {
  std::ofstream out(filename);
  NPNDatabaseSerializer().serialize(out, *this);
}

void NPNDatabaseSerializer::serialize(std::ostream &out, const NPNDatabase &obj) {
  storageSerializer.serialize(out, obj.storage);
}

NPNDatabase NPNDatabaseSerializer::deserialize(std::istream &in) {
  NPNDatabase result;
  result.storage = storageSerializer.deserialize(in);
  return result;
}

} // namespace eda::gate::optimizer
