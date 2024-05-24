//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/serializer.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/subnet_info.h"
#include "util/citerator.h"
#include "util/kitty_utils.h"
#include "util/serializer.h"

#include "kitty/kitty.hpp"

#include <fstream>
#include <sstream>
#include <vector>

namespace eda::gate::optimizer {

class NPNDB2ResultIterator : public ConstIterator<model::SubnetID> {
public:
  using NPNTransformation = eda::utils::NPNTransformation;
  using Subnet = eda::gate::model::Subnet;
  using SubnetID = model::SubnetID;
  using SubnetIDList = std::vector<SubnetID>;
  using SubnetInfoList = std::vector<SubnetInfo>;

  NPNDB2ResultIterator(const SubnetIDList &l, const NPNTransformation &t) :
                       transformation(t), list(l), ind(0) { }
  NPNDB2ResultIterator(const SubnetIDList &&l, const NPNTransformation &t) :
                       transformation(t), list(std::move(l)), ind(0) { }

  NPNDB2ResultIterator(const SubnetIDList &l,
                       const NPNTransformation &t,
                       const SubnetInfoList &il) :
                       transformation(t), list(l), ind(0), hasInfo(true),
                       infoList(il) { }

  bool isEnd() const override {
    return ind >= list.size();
  }

  bool next() override {
    if (isEnd()) {
      return false;
    }
    ind++;
    return isEnd();
  }

  SubnetID get() const override {
    if (isEnd()) {
      throw std::runtime_error("The iterator has reached end of the list");
    }
    return eda::utils::npnTransform(Subnet::get(list[ind]), transformation);
  }

  size_t size() const override {
    return list.size();
  }

  operator bool() const override {
    return !isEnd();
  }

  bool hasSubnetInfo() const {
    return hasInfo;
  }

  SubnetInfo getInfo() const {
    if (!hasInfo) {
      throw std::runtime_error("Iterator doesn't contain subnets info");
    }
    return infoList[ind];
  }
private:
  NPNTransformation transformation;
  SubnetIDList list;
  size_t ind;
  bool hasInfo = false;
  SubnetInfoList infoList;
};

/**
 * \brief Implements rewrite database which uses NPN matching to store nets.
 */
class NPNDatabase {
friend class NPNDatabaseSerializer;

public:
  using ResultIterator = NPNDB2ResultIterator;
  using NPNTransformation = utils::NPNTransformation;
  using Subnet = model::Subnet;
  using SubnetID = model::SubnetID;
  using SubnetIDList = std::vector<SubnetID>;
  using TT = kitty::dynamic_truth_table;

  virtual ~NPNDatabase() = default;

  /**
   * \brief Finds nets equivalent to representative function of *tt* NPN-class.
   * Returns tuple which consists of subnet references list of nets implementing
   * function of class representative, negation mask and permutation to
   * apply to representative.
   */
  virtual ResultIterator get(const TT &tt);
  virtual ResultIterator get(const Subnet &subnet);

  /**
   * \brief Push *bnet*'s NPN representative function net in database.
   */
  virtual NPNTransformation push(const SubnetID &id);

  virtual void erase(const TT &tt);

  static NPNDatabase importFrom(const std::string &filename);
  void exportTo(const std::string &filename) const;

protected:
  // Storage only contains NPN class representatives.
  std::map<TT, SubnetIDList> storage;
};

// Serializer for NPNStatDatabase class
class NPNDatabaseSerializer : public util::Serializer<NPNDatabase> {
public:
  using TT = NPNDatabase::TT;
  using SubnetList = std::vector<model::SubnetID>;

  void serialize(std::ostream &out, const NPNDatabase &obj);
  NPNDatabase deserialize(std::istream &in);
private:
  util::MapSerializer<TT,
                      SubnetList,
                      model::TTSerializer,
                      model::SubnetListSerializer> storageSerializer;
};

} // namespace eda::gate::optimizer
