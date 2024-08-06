//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/serializer.h"
#include "gate/optimizer/npndb.h"
#include "gate/optimizer/subnet_info.h"
#include "util/serializer.h"

#include <fstream>

namespace eda::gate::optimizer {

/**
 * \brief Extended NpnDatabase class.
 * Collects statistics and stores extra information in
 * `SubnetInfo` struct.
 */
class NpnStatDatabase : public NpnDatabase {
friend class NpnStatDatabaseSerializer;

public:

  using SubnetInfoList = std::vector<SubnetInfo>;

  ~NpnStatDatabase() override = default;

  ResultIterator get(const TT &tt) override;
  ResultIterator get(const Subnet &subnet) override;

  NpnTransformation push(const SubnetID &id, const SubnetInfo &subnetInfo);
  NpnTransformation push(const SubnetID &id) override;
  void erase(const TT &tt) override;

  // Get method which doesn't trigger access counter.
  ResultIterator getQuietly(const TT &tt);
  ResultIterator getQuietly(const Subnet &subnet);

  virtual void printDot(std::ostream &out, const TT &tt,
                        const std::string &name, const bool quiet = false) override;
  virtual void printInfo(std::ostream &out, const TT &tt, 
                        const bool quiet = false) override;

  virtual void printDotQuietly(std::ostream &out, const TT &tt,
                        const std::string &name);
  virtual void printDotFileQuietly(const TT &tt, const std::string &fileName,
                           const std::string &name);
  virtual void printInfoQuietly(std::ostream &out, const TT &tt);

  const std::unordered_map<TT, uint64_t> &getAccessCounter();

  // Loading to and from file
  static NpnStatDatabase importFrom(const std::string &filename);
  void exportTo(const std::string &filename);

protected:
  std::unordered_map<TT, SubnetInfoList> info;
  std::unordered_map<TT, uint64_t> accessCounter;

private:
  ResultIterator get(const TT &tt, bool quiet);
};

// Serializer for NpnStatDatabase class
class NpnStatDatabaseSerializer : public util::Serializer<NpnStatDatabase> {

public:
  using TT = NpnStatDatabase::TT;
  using SubnetList = std::vector<model::SubnetID>;
  using SubnetInfoList = std::vector<SubnetInfo>;

  void serialize(std::ostream &out, const NpnStatDatabase &obj);
  NpnStatDatabase deserialize(std::istream &in);

private:
  util::MapSerializer<TT,
                      SubnetList,
                      model::TTSerializer,
                      model::SubnetListSerializer> storageSerializer;
  util::MapSerializer<TT,
                      SubnetInfoList,
                      model::TTSerializer,
                      util::VectorSerializer<SubnetInfo>> infoSerializer;
  util::MapSerializer<TT, uint64_t> acSerializer;
};

} // namespace eda::gate::optimizer
