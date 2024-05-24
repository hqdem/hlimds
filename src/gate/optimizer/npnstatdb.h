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
 * \brief Extended NPNDatabase class.
 * Collects statistics and stores extra information in
 * `SubnetInfo` struct.
 */
class NPNStatDatabase : public NPNDatabase {
friend class NPNStatDatabaseSerializer;

public:

  using SubnetInfoList = std::vector<SubnetInfo>;

  ~NPNStatDatabase() override = default;

  ResultIterator get(const TT &tt) override;
  ResultIterator get(const Subnet &subnet) override;

  NPNTransformation push(const SubnetID &id, const SubnetInfo &subnetInfo);
  NPNTransformation push(const SubnetID &id) override;
  void erase(const TT &tt) override;

  // Get method which doesn't trigger access counter.
  ResultIterator getQuietly(const TT &tt);
  ResultIterator getQuietly(const Subnet &subnet);

  const std::map<TT, uint64_t> &getAccessCounter();

  // Loading to and from file
  static NPNStatDatabase importFrom(const std::string &filename);
  void exportTo(const std::string &filename);

protected:
  std::map<TT, SubnetInfoList> info;
  std::map<TT, uint64_t> accessCounter;

private:
  ResultIterator get(const TT &tt, bool quiet);
};

// Serializer for NPNStatDatabase class
class NPNStatDatabaseSerializer : public util::Serializer<NPNStatDatabase> {

public:
  using TT = NPNStatDatabase::TT;
  using SubnetList = std::vector<model::SubnetID>;
  using SubnetInfoList = std::vector<SubnetInfo>;

  void serialize(std::ostream &out, const NPNStatDatabase &obj);
  NPNStatDatabase deserialize(std::istream &in);

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
