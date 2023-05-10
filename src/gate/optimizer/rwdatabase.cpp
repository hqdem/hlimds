//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "rwdatabase.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace eda::gate::optimizer {

using BoundGNet = eda::gate::optimizer::BoundGNet;
using BoundGNetList = BoundGNet::BoundGNetList;
using Gate = eda::gate::model::Gate;
using GateList = std::vector<Gate::Id>;
using GateMap = std::map<Gate::Id, Gate::Id>;
using GateSymbol = eda::gate::model::GateSymbol;
using GNet = eda::gate::model::GNet;
using TruthTable = eda::gate::optimizer::TruthTable;

std::string SQLiteRWDatabase::serialize(const BoundGNetList &list) {
  std::stringstream ss;
  ss << list.size() << ' ';
  for (const auto &bGNet : list) {
    auto &inputBindings = bGNet.inputBindings;
    auto &outputBindings = bGNet.outputBindings;
    auto &inputDelays = bGNet.inputDelays;
    auto &net = bGNet.net;

    if (!net->isSorted()) {
      throw "Net isn't topologically sorted.";
    }

    // Inputs
    ss << inputBindings.size() << ' ';
    for (auto &inputId : inputBindings) {
      ss << inputId << ' ';
    }

    // Outputs
    ss << outputBindings.size() << ' ';
    for (auto &outputId : outputBindings) {
      ss << outputId << ' ';
    }

    // Inputs delay
    ss << inputDelays.size() << ' ';
    for (auto delay : inputDelays) {
      uint64_t ser = *( (uint64_t*)&delay );
      ss << ser << ' ';
    }

    // Net
    ss << net->gates().size() << ' ';
    for (auto *gate : net->gates()) {
      ss << (uint16_t)gate->func() << ' ' << gate->id() << ' ';
      ss << gate->inputs().size() << ' ';
      for (auto signal : gate->inputs()) {
        ss << signal.node() << ' ';
      }
    }
  }
  return ss.str();
}

BoundGNetList SQLiteRWDatabase::deserialize(const std::string &str) {
  std::stringstream ss;
  ss.str(str);
  BoundGNetList result;
  size_t size;
  ss >> size;
  for (size_t i = 0; i < size; i++) {
    BoundGNet bGNet;

    size_t inputBindingsSize;
    size_t outputBindingsSize;
    size_t inputDelaysSize;

    // Inputs
    ss >> inputBindingsSize;
    std::map<Gate::Id, uint32_t> rInputs;

    for (size_t j = 0; j < inputBindingsSize; j++) {
      Gate::Id gateId;
      ss >> gateId;
      bGNet.inputBindings.push_back(gateId);
      rInputs[gateId] = j;
    }

    // Outputs
    ss >> outputBindingsSize;
    std::map<Gate::Id, uint32_t> rOutputs;

    for (size_t j = 0; j < outputBindingsSize; j++) {
      Gate::Id gateId;
      ss >> gateId;
      bGNet.outputBindings.push_back(gateId);
      rOutputs[gateId] = j;
    }

    // Inputs delay
    ss >> inputDelaysSize;
    for (size_t j = 0; j < inputDelaysSize; j++) {
      uint64_t ser;
      ss >> ser;
      double delay = *( (double*)&ser );
      bGNet.inputDelays.push_back(delay);
    }

    // Net
    size_t gateCount;
    ss >> gateCount;
    std::shared_ptr<GNet> net = std::make_shared<GNet>();

    GateMap oldNewMap;

    for (size_t j = 0; j < gateCount; j++) {
      GateSymbol::Value func;
      Gate::Id id;
      size_t inputCount;
      Gate::SignalList inputBindings;
      uint16_t intFunc;

      ss >> intFunc >> id >> inputCount;
      func = (GateSymbol::Value)intFunc;

      Gate::Id newId;
      if (inputCount) {
        for (size_t k = 0; k < inputCount; k++) {
          Gate::Id inputId;
          ss >> inputId;
          Gate::Id newInputId = oldNewMap[inputId];
          inputBindings.push_back(Gate::Signal::always(newInputId));
        }
        newId = net->addGate(func, inputBindings);
        oldNewMap[id] = newId;
      } else {
        newId = net->addGate(func, inputBindings);
        oldNewMap[id] = newId;
        bGNet.inputBindings[rInputs[id]] = newId;
      }

      if (func == GateSymbol::OUT) {
        bGNet.outputBindings[rOutputs[id]] = newId;
      }
    }
    bGNet.net = net;
    bGNet.net->sortTopologically();

    result.push_back(bGNet);
  }

  return result;
}

bool SQLiteRWDatabase::dbContainsRWTable() {
  assert(_isOpened);
  _selectResult.clear();
  std::string sql = "SELECT name FROM sqlite_master WHERE " \
                    "type='table' AND name=?;";
  _rc = sqlite3_bind_exec(_db, sql.c_str(), selectSQLCallback,
                          (void*)(&_selectResult),
                          SQLITE_BIND_TEXT(_dbTableName.c_str()),
                          SQLITE_BIND_END);
  if (_rc != SQLITE_OK) {
    throw "Can't use db.";
  }
  return !_selectResult.empty();
}

void SQLiteRWDatabase::linkDB(const std::string &path) {
  _rc = sqlite3_open(path.c_str(), &_db);

  if (_rc != SQLITE_OK) {
    throw "Can't open database.";
  }
  _isOpened = true;

  if (!dbContainsRWTable()) {
    std::string sql = "CREATE TABLE " + _dbTableName + " (" + _dbKeyName + " "
                      + _dbKeyType + " PRIMARY KEY, " + _dbValueName + " " +
                      _dbValueType + ")";
    _rc = sqlite3_exec(_db, sql.c_str(), nullptr, 0, &_zErrMsg);
    if (_rc != SQLITE_OK) {
      std::cout << sqlite3_errmsg(_db) << '\n';
      throw "Can't create table.";
    }
  }

  _pathDB = path;
  _isLinked = true;
  sqlite3_close(_db);
  _isOpened = false;
}

void SQLiteRWDatabase::openDB() {
  if (!_isLinked) {
    throw "No database was linked.";
  }
  _rc = sqlite3_open(_pathDB.c_str(), &_db);
  if (_rc != SQLITE_OK) {
    throw "Can't open database.";
  }
  _isOpened = true;
}

void SQLiteRWDatabase::closeDB() {
  assert(_isLinked);
  sqlite3_close(_db);
  _isOpened = false;
}

bool SQLiteRWDatabase::contains(const TruthTable key) {
  if (_storage.find(key.raw()) != _storage.end()) {
    return true;
  }
  if (_isOpened) {
    _selectResult.clear();
    std::string sql = "SELECT * FROM " + _dbTableName + " " \
                      "WHERE " + _dbKeyName + "=?";
    _rc = sqlite3_bind_exec(_db, sql.c_str(), selectSQLCallback,
                            (void*)(&_selectResult),
                            SQLITE_BIND_INT64(key.raw()),
                            SQLITE_BIND_END);
    if (_rc != SQLITE_OK) {
      std::cout << sqlite3_errmsg(_db) << '\n';
      throw "Can't select.";
    }
    return !_selectResult.empty();
  }
  return false;
}

BoundGNetList SQLiteRWDatabase::get(const TruthTable key) {
  if (_storage.find(key.raw()) != _storage.end()) {
    return _storage[key.raw()];
  }
  if (_isOpened) {
    _selectResult.clear();
    std::string sql = "SELECT * FROM " + _dbTableName + " WHERE " +
                      _dbKeyName + "=?";
    _rc = sqlite3_bind_exec(_db, sql.c_str(), selectSQLCallback,
                            (void*)(&_selectResult),
                            SQLITE_BIND_INT64(key.raw()),
                            SQLITE_BIND_END);
    if (_rc != SQLITE_OK) {
      std::cout << sqlite3_errmsg(_db) << '\n';
      throw "Can't select.";
    }
    if (!_selectResult.empty()) {
      BoundGNetList deser = deserialize(_selectResult[0].second);
      set(key, deser);
      return deser;
    }
  }
  return BoundGNetList();
}

void SQLiteRWDatabase::insertIntoDB(const TruthTable key,
                                    const BoundGNetList &value) {
  assert(_isOpened);
  std::string ser = serialize(value);

  std::string sql = "INSERT INTO " + _dbTableName + " (" +
                    _dbKeyName + ", " + _dbValueName + ") " +
                    "VALUES (?,?)";
  _rc = sqlite3_bind_exec(_db, sql.c_str(), nullptr, nullptr,
                          SQLITE_BIND_INT64(key.raw()),
                          SQLITE_BIND_TEXT(ser.c_str()),
                          SQLITE_BIND_END);
  if (_rc != SQLITE_OK) {
    std::cout << sqlite3_errmsg(_db) << '\n';
    throw "Can't insert.";
  }
}

void SQLiteRWDatabase::updateInDB(const TruthTable key,
                                  const BoundGNetList &value) {
  assert(_isOpened);
  std::string ser = serialize(value);
  std::string sql = "UPDATE " + _dbTableName + " SET " + _dbValueName +
                    "=? WHERE " + _dbKeyName + "=?";
  _rc = sqlite3_bind_exec(_db, sql.c_str(), nullptr, nullptr,
                          SQLITE_BIND_TEXT(ser.c_str()),
                          SQLITE_BIND_INT64(key.raw()),
                          SQLITE_BIND_END);
  if (_rc != SQLITE_OK) {
    std::cout << sqlite3_errmsg(_db) << '\n';
    throw "Can't update.";
  }
}

void SQLiteRWDatabase::deleteFromDB(const TruthTable key) {
  assert(_isOpened);
  std::string sql = "DELETE FROM " + _dbTableName + " WHERE " + _dbKeyName + "=?";
  _rc = sqlite3_bind_exec(_db, sql.c_str(), nullptr, nullptr,
                          SQLITE_BIND_INT64(key.raw()),
                          SQLITE_BIND_END);
  if (_rc != SQLITE_OK) {
    std::cout << sqlite3_errmsg(_db) << '\n';
    throw "Can't delete.";
  }
}

BoundGNetList SQLiteRWDatabase::getFromDB(const TruthTable key) {
  _selectResult.clear();
  std::string sql = "SELECT * FROM " + _dbTableName + " WHERE " +
                    _dbKeyName + "=?";
  _rc = sqlite3_bind_exec(_db, sql.c_str(), selectSQLCallback,
                          (void*)(&_selectResult),
                          SQLITE_BIND_INT64(key.raw()),
                          SQLITE_BIND_END);
  if (_rc != SQLITE_OK) {
    std::cout << sqlite3_errmsg(_db) << '\n';
    throw "Can't select.";
  }
  if (!_selectResult.empty()) {
    BoundGNetList deser = deserialize(_selectResult[0].second);
    set(key, deser);
    return deser;
  }
  return BoundGNetList();
}

bool SQLiteRWDatabase::dbContains(const TruthTable key) {
  assert(_isOpened);
  return !getFromDB(key).empty();
}

void SQLiteRWDatabase::pushInDB(const TruthTable key, const BoundGNet &value) {
  assert(_isOpened);
  BoundGNetList newValue = getFromDB(key);
  if (newValue.empty()) {
    insertIntoDB(key, {value});
  } else {
    newValue.push_back(value);
    updateInDB(key, newValue);
  }
}

void SQLiteRWDatabase::pushInDB(const TruthTable key, const BoundGNetList &value) {
  assert(_isOpened);
  BoundGNetList newValue = getFromDB(key);
  if (newValue.empty()) {
    insertIntoDB(key, {value});
  } else {
    for (const auto &bnet : value) {
      newValue.push_back(bnet);
    }
    updateInDB(key, newValue);
  }
}

} // namespace eda::gate::optimizer
