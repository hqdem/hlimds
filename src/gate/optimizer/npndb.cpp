//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/printer/net_printer.h"
#include "gate/optimizer/npndb.h"
#include "gate/translator/logdb.h"

namespace eda::gate::optimizer {

using TT = NpnDatabase::TT;

NpnDatabase::ResultIterator NpnDatabase::get(const TT &tt) {
  const size_t nVars = tt.num_vars();
  const TT ttk = nVars < nInputs ? kitty::extend_to(tt, nInputs) : tt;
  auto config = kitty::exact_npn_canonization(ttk);
  NpnTransformation t = util::getTransformation(config);
  const auto &canonTT = util::getTT(config);
  return ResultIterator(storage[canonTT], util::inverse(t), nVars);
}

NpnDatabase::ResultIterator NpnDatabase::get(const Subnet &subnet) {
  TT tt = model::evaluate(subnet)[0];
  return get(tt);
}

void NpnDatabase::printDot(std::ostream &out, const TT &tt,
                           const std::string &name, const bool quiet) {
  NpnDatabase::ResultIterator iterator = get(tt);
  if (iterator.isEnd()) {
    return;
  }
  model::print(out, model::DOT, name, Subnet::get(iterator.get()));
}

void NpnDatabase::printDotFile(const TT &tt, const std::string &fileName,
                               const std::string &name, const bool quiet) {
  std::ofstream out;
  out.open(fileName);
  if (out.is_open()) {
    printDot(out, tt, name, quiet);
  }
  out.close();
}

void NpnDatabase::printInfo(std::ostream &out, const TT &tt, const bool quiet) {
  NpnDatabase::ResultIterator iterator = get(tt);
  if (iterator.isEnd()) {
    return;
  }
  Subnet &subnet1 = Subnet::get(iterator.get());
  printInfoSub(out, subnet1);
}

void NpnDatabase::printInfoSub(std::ostream &out, const Subnet &subnet) {
  out << "nIn: " << subnet.getInNum() << "\n";
  out << "nOut: " << subnet.getOutNum() << "\n";
  out << "nEntry: " << subnet.size() << "\n";
}

NpnDatabase::NpnTransformation NpnDatabase::push(const SubnetID &id) {
  TT tt = model::evaluate(Subnet::get(id))[0];
  auto config = kitty::exact_npn_canonization(tt);
  NpnTransformation t = util::getTransformation(config);
  auto newId = util::npnTransform(Subnet::get(id), t);
  storage[std::get<0>(config)].push_back(newId);
  return t;
}

void NpnDatabase::erase(const TT &tt) {
  storage.erase(tt);
}

NpnDatabase NpnDatabase::importFrom(const std::string &filename) {
  translator::LogDbTranslator translator;
  return translator.translate(filename);
}

void NpnDatabase::exportTo(const std::string &filename) const {
  std::ofstream out(filename);
  if (!out.is_open()) {
    throw std::runtime_error("Error of opening file to export\n");
  }

  for (const auto &ttToSubnetList : storage) {
    for (const auto &subnetID : ttToSubnetList.second) {
      model::print(out, model::Format::LOGDB, model::Subnet::get(subnetID));
      out << '\n';
    }
  }
}

void NpnDatabaseSerializer::serialize(std::ostream &out,
                                      const NpnDatabase &obj) {
  storageSerializer.serialize(out, obj.storage);
}

NpnDatabase NpnDatabaseSerializer::deserialize(std::istream &in) {
  NpnDatabase result;
  result.storage = storageSerializer.deserialize(in);
  return result;
}

} // namespace eda::gate::optimizer
