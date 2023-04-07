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

using BoundGNetList = eda::gate::optimizer::RWDatabase::BoundGNetList;
using Gate = eda::gate::model::Gate;
using GateList = std::vector<Gate::Id>;
using GateSymbol = eda::gate::model::GateSymbol;
using GNet = eda::gate::model::GNet;
using RWDatabase = eda::gate::optimizer::RWDatabase;
using SQLiteRWDatabase = eda::gate::optimizer::SQLiteRWDatabase;

namespace eda::gate::optimizer {

std::string SQLiteRWDatabase::serialize(const BoundGNetList &list) {
  std::stringstream ss;
  ss << list.size() << ' ';
  for (const auto &bGNet : list) {
    auto inputs = bGNet.inputs;
    auto outputs = bGNet.outputs;
    auto net = bGNet.net;
    if (!net->isSorted()) {
      throw "Net isn't topologically sorted.";
    }

    ss << inputs.size() << ' ';
    for (auto &inputId : inputs) {
      ss << inputId << ' ';
    }

    ss << outputs.size() << ' ';
    for (auto &outputId : outputs) {
      ss << outputId << ' ';
    }

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

    size_t inputsSize;
    size_t outputsSize;

    ss >> inputsSize;
    std::map<Gate::Id, uint32_t> rInputs;

    for (size_t j = 0; j < inputsSize; j++) {
      Gate::Id gateId;
      ss >> gateId;
      bGNet.inputs.push_back(gateId);
      rInputs[gateId] = j;
    }

    ss >> outputsSize;
    std::map<Gate::Id, uint32_t> rOutputs;

    for (size_t j = 0; j < outputsSize; j++) {
      Gate::Id gateId;
      ss >> gateId;
      bGNet.outputs.push_back(gateId);
      rOutputs[gateId] = j;
    }

    size_t gateCount;
    ss >> gateCount;
    std::shared_ptr<GNet> net = std::make_shared<GNet>();

    GateMap oldNewMap;

    for (size_t j = 0; j < gateCount; j++) {
      GateSymbol::Value func;
      Gate::Id id;
      size_t inputCount;
      Gate::SignalList inputs;
      uint16_t intFunc;

      ss >> intFunc >> id >> inputCount;
      func = (GateSymbol::Value)intFunc;

      Gate::Id newId;
      if (inputCount) {
        for (size_t k = 0; k < inputCount; k++) {
          Gate::Id inputId;
          ss >> inputId;
          Gate::Id newInputId = oldNewMap[inputId];
          inputs.push_back(Gate::Signal::always(newInputId));
        }
        newId = net->addGate(func, inputs);
        oldNewMap[id] = newId;
      } else {
        newId = net->addGate(func, inputs);
        oldNewMap[id] = newId;
        bGNet.inputs[rInputs[id]] = newId;
      }

      if (func == GateSymbol::OUT) {
        bGNet.outputs[rOutputs[id]] = newId;
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

} // namespace eda::gate::optimizer
