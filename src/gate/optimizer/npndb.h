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
#include "util/truth_table.h"

#include "kitty/kitty.hpp"

#include <fstream>
#include <sstream>
#include <vector>

namespace eda::gate::optimizer {

class NpnDb2ResultIterator : public ConstIterator<model::SubnetID> {
public:
  using NpnTransformation = eda::util::NpnTransformation;
  using Subnet = eda::gate::model::Subnet;
  using SubnetID = model::SubnetID;
  using SubnetIDList = std::vector<SubnetID>;
  using SubnetInfoList = std::vector<SubnetInfo>;

  NpnDb2ResultIterator(const SubnetIDList &l,
                       const NpnTransformation &t,
                       uint8_t nInUsed = -1) :
                       transformation(t), list(l), ind(0), nInUsed(nInUsed) { }

  NpnDb2ResultIterator(const SubnetIDList &&l,
                       const NpnTransformation &t,
                       uint8_t nInUsed = -1) :
                       transformation(t), list(std::move(l)), ind(0),
                       nInUsed(nInUsed) { }

  NpnDb2ResultIterator(const SubnetIDList &l,
                       const NpnTransformation &t,
                       const SubnetInfoList &il,
                       uint8_t nInUsed = -1) :
                       transformation(t), list(l), ind(0), hasInfo(true),
                       infoList(il), nInUsed(nInUsed) { }

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
    const auto &subnet = Subnet::get(list[ind]);
    return eda::util::npnTransform(subnet, transformation, nInUsed);
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
  NpnTransformation transformation;
  SubnetIDList list;
  size_t ind;
  bool hasInfo = false;
  SubnetInfoList infoList;
  uint8_t nInUsed = -1;
};

/**
 * \brief Implements rewrite database which uses Npn matching to store nets.
 */
class NpnDatabase {
friend class NpnDatabaseSerializer;

public:
  using ResultIterator = NpnDb2ResultIterator;
  using NpnTransformation = util::NpnTransformation;
  using Subnet = model::Subnet;
  using SubnetID = model::SubnetID;
  using SubnetIDList = std::vector<SubnetID>;
  using TT = kitty::dynamic_truth_table;

  NpnDatabase(uint8_t nInputs) : nInputs(nInputs) { }
  NpnDatabase() = default;
  virtual ~NpnDatabase() = default;

  /**
   * \brief Finds nets equivalent to representative function of *tt* Npn-class.
   * Returns tuple which consists of subnet references list of nets implementing
   * function of class representative, negation mask and permutation to
   * apply to representative.
   */
  virtual ResultIterator get(const TT &tt);
  virtual ResultIterator get(const Subnet &subnet);

  /**
   * \brief Finds nets equivalent to representative function of *tt* Npn-class, 
   * creates a DOT representation for the net, and prints the representation in buffer.
   * \param out the buffer into which the result is being output.
   * \param tt Truth table.
   * \param name Header of DOT representation.
   * \param quiet Flag for quiet version. Used only in child class
   */
  virtual void printDot(std::ostream &out, const TT &tt,
                        const std::string &name, const bool quiet = false);
                        
  /**
   * \brief Finds nets equivalent to representative function of *tt* Npn-class, 
   * creates a DOT representation for the net, and save the representation in file.
   * \param tt Truth table.
   * \param fileName File into which the result is being saved.
   * \param name Header of DOT representation.
   * \param quiet Flag for quiet version. Used only in child class
   */
  virtual void printDotFile(const TT &tt, const std::string &fileName,
                            const std::string &name, const bool quiet = false);

  /**
   * \brief Finds nets equivalent to representative function of *tt* (or *subnet*) Npn-class, 
   * and prints in buffer the information about Subnet (INs, OUTSs, ENTRYs).
   * \param out the buffer into which the result is being output.
   * \param tt Truth table.
   * \param quiet Flag for quiet version. Used only in child class
   */
  virtual void printInfo(std::ostream &out, const TT &tt, const bool quiet = false);
  static void printInfoSub(std::ostream &out, const Subnet &subnet);

  /**
   * \brief Push *bnet*'s Npn representative function net in database.
   */
  virtual NpnTransformation push(const SubnetID &id);

  virtual void erase(const TT &tt);

  static NpnDatabase importFrom(const std::string &filename);
  void exportTo(const std::string &filename) const;

  /// @brief Sets the number of inputs of the Subnets in the database.
  void setInNum(uint8_t inNum) { nInputs = inNum; }

protected:
  // Storage only contains Npn class representatives.
  std::unordered_map<TT, SubnetIDList> storage;
  uint8_t nInputs = 0;
};

// Serializer for NpnStatDatabase class
class NpnDatabaseSerializer : public util::Serializer<NpnDatabase> {
public:
  using TT = NpnDatabase::TT;
  using SubnetList = std::vector<model::SubnetID>;

  void serialize(std::ostream &out, const NpnDatabase &obj);
  NpnDatabase deserialize(std::istream &in);
private:
  util::MapSerializer<TT,
                      SubnetList,
                      model::TTSerializer,
                      model::SubnetListSerializer> storageSerializer;
};

} // namespace eda::gate::optimizer
