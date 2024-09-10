//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/readcells_iface.h"

#include <cfloat>

namespace eda::gate::library {

// TODO: move this Expr functions to std::string into ReadCells

std::string binOpToString(
    const Expr *expr, const std::string &op);

std::string exprToString(const Expr *expr) {
  if (!expr) return "";

  std::stringstream ss;

  switch (expr->kind) {
    case EK_Identifier:
      ss << expr->name;
      break;
    case EK_Literal:
      ss << (expr->bval ? "1" : "0");
      break;
    case EK_Subscript:
      ss << expr->name << "[" << exprToString(expr->opnd) << "]";
      break;
    case EK_Not:
      ss << "!(" << exprToString(expr->opnd) << ")";
      break;
    case EK_Xor:
      ss << binOpToString(expr, "^");
      break;
    case EK_And:
      ss << binOpToString(expr, "&");
      break;
    case EK_Or:
      ss << binOpToString(expr, "|");
      break;
    default:
      ss << "unknown";
      break;
  }
  return ss.str();
}

std::string binOpToString(
    const Expr *expr, const std::string &op) {
  return "(" + exprToString(expr->binop.lhs) + op +
               exprToString(expr->binop.rhs) + ")";
}

void getIdsInExpr(std::map<std::string, int> &ids, const Expr *expr) {
  if (expr->kind == EK_Identifier) {
    ids[std::string(expr->name)] = 1;
  } else if (expr->kind == EK_Not || expr->kind == EK_Subscript) {
    getIdsInExpr(ids, expr->opnd);
  } else if (expr->kind == EK_Xor ||
             expr->kind == EK_And || expr->kind == EK_Or) {
    getIdsInExpr(ids, expr->binop.lhs);
    getIdsInExpr(ids, expr->binop.rhs);
  }
}

bool checkIdsInExpr(const Expr *expr, const std::vector<std::string> &ids) {
  std::map<std::string, int> existingIds;
  getIdsInExpr(existingIds, expr);
  for (const auto &id : ids) {
    if (existingIds.find(id) == existingIds.end()) {
      return false;
    }
  }
  return true;
}

inline bool isInputPin(const Pin &pin) {
  return (pin.getIntegerAttribute("direction", 10) & 0x1) != 0;
}

inline bool isOutputPin(const Pin &pin) {
  return (pin.getIntegerAttribute("direction", 10) & 0x2) != 0;
}

const Pin *ReadCellsIface::getOutputPin(
    const std::string &name, uint number) {
  const Pin *output = nullptr;
  uint currentPin = 0;
  for (const auto &pin : library.getCell(name)->getPins()) {
    if (isOutputPin(pin) && (currentPin++ == number)) {
      output = &pin;
      break;
    }
  }
  return output;
}

const Expr *ReadCellsIface::getExprFunction(
    const std::string &name, uint number) {
  const auto *pin = getOutputPin(name, number);
  const Expr *func = nullptr;
  if (pin != nullptr) {
    func = pin->getBexprAttribute("function");
  }
  return func;
}

const std::string ReadCellsIface::getStringFunction(
    const std::string &name, uint number) {
  const Expr *func = getExprFunction(name, number);
  std::string result;
  if (func != nullptr) {
    result = exprToString(func);
  }
  return result;
}

std::vector<std::string> ReadCellsIface::getCells() {
  std::vector<std::string> cells;
  for (const auto &cell : library.getCells()) {
    cells.push_back(std::string(cell.getName()));
  }
  return cells;
}

model::CellType::PortVector ReadCellsIface::getPorts(const std::string &name) {
  model::CellType::PortVector ports;
  size_t index{0};
  for (const auto &pin : library.getCell(name)->getPins()) {
    ports.emplace_back(std::string(pin.getName()), 1, isInputPin(pin), index++);
  }
  return ports; 
}

std::vector<std::string> ReadCellsIface::getInputs(const std::string &name) {
  std::vector<std::string> inputs;
  for (const auto &pin : library.getCell(name)->getPins()) {
    if (isInputPin(pin)) {
      inputs.push_back(std::string(pin.getName()));
    }
  }
  return inputs;
}

std::vector<std::string> ReadCellsIface::getOutputs(const std::string &name) {
  std::vector<std::string> outputs;
  for (const auto &pin : library.getCell(name)->getPins()) {
    if (isOutputPin(pin)) {
      outputs.push_back(std::string(pin.getName()));
    }
  }
  return outputs;
}

kitty::dynamic_truth_table ReadCellsIface::getFunction(
    const std::string &name, uint number) {
  const auto &inputs = getInputs(name); // TODO: assert inputs.size() != 0
  kitty::dynamic_truth_table truthTable(inputs.size());
  kitty::create_from_formula(truthTable,
    getStringFunction(name, number), inputs);
  return truthTable;
}

bool ReadCellsIface::isCombCell(const std::string &name){
  const auto &inputs = getInputs(name);
  const auto &outputs = getOutputs(name);
  const auto *cell = library.getCell(name);
  bool funcVerify = (inputs.size() == 0 && outputs.size() != 0) ||
                    ((getExprFunction(name) != nullptr) &&
                    checkIdsInExpr(getExprFunction(name), getInputs(name)));

  return !cell->hasAttribute("ff") &&
         !cell->hasAttribute("latch") &&
         std::find(outputs.begin(), outputs.end(), "CLK") == outputs.end() &&
         funcVerify &&
         outputs.size() > 0/*== 1*/;
}

float ReadCellsIface::getArea(const std::string &name) {
  const auto *cell = library.getCell(name);
  return cell ?
         cell->getFloatAttribute("area", FLT_MAX) : FLT_MAX;
}

float ReadCellsIface::getLeakagePower(const std::string &name) {
  const auto *cell = library.getCell(name);
  return cell ?
         cell->getFloatAttribute("cell_leakage_power", FLT_MAX) : FLT_MAX;
}

std::vector<ReadCellsIface::Delay> ReadCellsIface::getDelay(
    const std::string &name,
    const std::vector<float> &inputTransTime,
    const float outputCap) {
  std::vector<ReadCellsIface::Delay> delay;
  auto ins = getInputs(name);
  for (size_t i = 0; i < ins.size(); i++) {
    delay.push_back(getDelay(name,
                             ins.at(i),
                             inputTransTime.at(i),
                             outputCap));
  }
  return delay;
}

float ReadCellsIface::getLutValue(const LookupTable *lut,
                                          const std::vector<std::size_t> &paramsID) {
  if (paramsID.empty() || lut->getIndicesSize() != paramsID.size()) {
    throw std::invalid_argument("Invalid search parameters or lookup table.");
  }

  size_t index = paramsID.back();
  size_t stride = 1;

  auto axis = lut->end();
  axis--;

  // The loop starts from the second-to-last element since we have already
  // calculated for the last one.
  for (size_t i = lut->getIndicesSize() - 2; i != static_cast<size_t>(-1); i--) {
    axis--;
    stride *= axis->values.size();
    index += stride * paramsID.at(i);
  }
  return lut->getValues().at(index);
}

float ReadCellsIface::getTwoAxisLutInterValue(
    const LookupTable *lut,
    const std::vector<InterParamIDs> &paramsID,
    const std::vector<float> &searchParams) {
  auto axisIterator = lut->begin();
  float x1 = axisIterator->values.at(paramsID.at(0).lowerID);
  float x2 = axisIterator->values.at(paramsID.at(0).upperID);

  axisIterator++;
  float y1 = axisIterator->values.at(paramsID.at(1).lowerID);
  float y2 = axisIterator->values.at(paramsID.at(1).upperID);

  float q11 = getLutValue(lut, {paramsID.at(0).lowerID, paramsID.at(1).lowerID});
  float q12 = getLutValue(lut, {paramsID.at(0).lowerID, paramsID.at(1).upperID});
  float q21 = getLutValue(lut, {paramsID.at(0).upperID, paramsID.at(1).lowerID});
  float q22 = getLutValue(lut, {paramsID.at(0).upperID, paramsID.at(1).upperID});

  float r1 = ((x2 - searchParams.at(0)) / (x2 - x1)) * q11 + ((searchParams.at(0) - x1) / (x2 - x1)) * q21;
  float r2 = ((x2 - searchParams.at(0)) / (x2 - x1)) * q12 + ((searchParams.at(0) - x1) / (x2 - x1)) * q22;
  float p = ((y2 - searchParams.at(1)) / (y2 - y1)) * r1 + ((searchParams.at(1) - y1) / (y2 - y1)) * r2;

  return p;
}

float ReadCellsIface::getLutInterValue(
    const LookupTable *lut,
    const std::vector<InterParamIDs> &paramsID,
    const std::vector<float> &searchParams) {
  switch (searchParams.size()) {
    case 2:
      return getTwoAxisLutInterValue(lut, paramsID, searchParams);
      break;
  }
  return FLT_MAX;
}

float ReadCellsIface::getValue(const LookupTable* lut,
                               const std::vector<float> &searchParams) {
  std::vector<InterParamIDs> paramsIndices;
  std::vector<size_t> searParamIndices;
  searParamIndices.reserve(lut->getIndicesSize());
  searParamIndices.resize(lut->getIndicesSize(), SIZE_MAX);
  auto axisIterator = lut->begin();

  for (const auto &param : searchParams) {
    InterParamIDs paramsID;

    for (size_t i = 0; i < axisIterator->values.size(); ++i) {
      auto value = axisIterator->values[i];

      if (value == param) {
        searParamIndices.at(paramsIndices.size() - 1) = i;
        break;
      }

      if (value > param) {
        paramsID.upperID = i;
        break;
      }

      paramsID.lowerID = i;
    }
    paramsIndices.push_back(paramsID);
    axisIterator++;
  }

  bool needInter = false;
  for (const auto &id : searParamIndices) {
    if (id == SIZE_MAX) {
      needInter = true;
      break;
    }
  }
  
  if (needInter) {
    return getLutInterValue(lut, paramsIndices, searchParams);
  }

  return getLutValue(lut, searParamIndices);
}

ReadCellsIface::Delay ReadCellsIface::getDelay(
    const std::string &name,
    const std::string &relPin,
    const float inputTransTime,
    const float outputCap) {
  Delay delay;
  std::vector<float> searchParams = {inputTransTime,outputCap};
  const auto *cell = library.getCell(name);
  const auto *pin = cell->getPin(getOutputs(name).at(0));
  const auto *timing = pin->getTiming(relPin);

  delay.cellFall = getValue(timing->getLut("cell_fall"),
                            searchParams);
  delay.cellRise = getValue(timing->getLut("cell_rise"),
                            searchParams);
  delay.fallTransition = getValue(timing->getLut("fall_transition"),
                                  searchParams);
  delay.riseTransition = getValue(timing->getLut("rise_transition"),
                                  searchParams);
  return delay;
}

} // namespace eda::gate::library
