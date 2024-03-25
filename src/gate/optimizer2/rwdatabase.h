//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model2/subnet.h"
#include "util/kitty_utils.h"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <functional>

namespace eda::gate::optimizer {

/**
* \brief Implements storage that contains Subnets for rewriting.
* \author <a href="mailto:mrpepelulka@gmail.com">Rustamkhan Ramaldanov</a>
*/
class RWDatabase2 {
public:
  using Subnet = model::Subnet;
  using SubnetID = model::SubnetID;
  using SubnetIDList = std::vector<SubnetID>;
  using TT = kitty::dynamic_truth_table;

  virtual bool contains(const TT key) {
    return (storage.find(key) != storage.end());
  }

  virtual SubnetIDList get(const TT key) {
    return contains(key) ? storage[key] : SubnetIDList();
  }

  virtual void set(const TT key, const SubnetIDList &value) {
    storage[key] = value;
  }

  virtual void erase(const TT key) {
    storage.erase(key);
  }

  virtual bool empty() {
    return storage.empty();
  }

  virtual void push(const TT key, const SubnetID& id) {
    if (contains(key)) {
      storage[key].emplace_back(id);
    } else {
      storage[key] = { id };
    }
  }

  virtual void push(const TT key, const SubnetIDList &value) {
    if (contains(key)) {
      for (const auto &id : value) {
        storage[key].push_back(id);
      }
    } else {
      storage[key] = value;
    }
  }

  virtual void clear() {
    storage.clear();
  }

  virtual ~RWDatabase2() { }

protected:
  std::map<TT, SubnetIDList> storage;

};

} // namespace eda::gate::optimizer

