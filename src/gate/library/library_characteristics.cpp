//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library_characteristics.h"
#include "gate/library/library_parser.h"
#include <float.h>

namespace eda::gate::library {

std::string LibraryCharacteristics::binOpToString(const Expr *lhs,
                                                  const std::string &op,
                                                  const Expr *rhs) {
  return "(" + exprToString(lhs) + op + exprToString(rhs) + ")";
}

std::string LibraryCharacteristics::exprToString(const Expr *expr) {
  if (!expr) return "";

  std::stringstream ss;

  switch (expr->kind) {
    case EK_Identifier:
      ss << expr->name;
      break;
    case EK_Literal:
      break;
    case EK_Subscript:
      ss << expr->name << "[" << exprToString(expr->opnd) << "]";
      break;
    case EK_Not:
      ss << "!(" << exprToString(expr->opnd) << ")";
      break;
    case EK_Xor:
      ss << binOpToString(expr->binop.lhs, "^", expr->binop.rhs);
      break;
    case EK_And:
      ss << binOpToString(expr->binop.lhs, "&", expr->binop.rhs);
      break;
    case EK_Or:
      ss << binOpToString(expr->binop.lhs, "|", expr->binop.rhs);
      break;
    default:
      ss << "unknown";
      break;
  }
  return ss.str();
}

bool LibraryCharacteristics::areIdsInExpr(
    const std::string &expr, const std::vector<std::string> &ids) {
  std::set<std::string> uniqueIds(ids.begin(), ids.end());
  std::regex wordRegex(R"(\b[a-zA-Z0-9]+\b)");
  std::sregex_iterator next(expr.begin(), expr.end(), wordRegex);
  std::sregex_iterator end;
  while (next != end) {
    std::smatch match = *next;
    std::string token = match.str();
    if (uniqueIds.find(token) == uniqueIds.end()) {
      return false;
    }
    next++;
  }

  return true;
}

std::vector<std::string> LibraryCharacteristics::getFunctions(const std::string &name) {
  std::vector<std::string> funcs;
  for (const auto& pin : LibraryParser::get().getLibrary().getCell(name)->getPins()) {
    if (pin.getIntegerAttribute("direction", 10) & (1 << 1)) {
      if (pin.hasAttribute("function")) {
        const auto* func = pin.getBexprAttribute("function");
        if (func != nullptr) {
          funcs.push_back(exprToString(func));
        }
      }
    }
  }
  return funcs;
}

std::vector<std::string> LibraryCharacteristics::getCells() {
  std::vector<std::string> cells;
  for (const auto &cell : LibraryParser::get().getLibrary().getCells()) {
    cells.push_back(std::string(cell.getName()));
  }
  return cells;
}

std::vector<std::string> LibraryCharacteristics::getInputs(const std::string &name) {
  std::vector<std::string> inputs;

  for (const auto& pin : LibraryParser::get().getLibrary().getCell(name)->
      getPins()) {
    if (pin.getIntegerAttribute("direction", 10) & (1 << 0)) {
      inputs.push_back(std::string(pin.getName()));
    }
  }
  return inputs;
}

std::vector<std::string> LibraryCharacteristics::getOutputs(const std::string &name) {
  std::vector<std::string> outputs;
  for (const auto& pin : LibraryParser::get().getLibrary().getCell(name)->
      getPins()) {
    if (pin.getIntegerAttribute("direction", 10) & (1 << 1)) {
      outputs.push_back(std::string(pin.getName()));
    }
  }
  return outputs;
}

kitty::dynamic_truth_table LibraryCharacteristics::getFunction(const std::string &name) {
  kitty::dynamic_truth_table truthTable(getInputs(name).size());
  kitty::create_from_formula(truthTable, getFunctions(name).at(0), getInputs(name));
  return truthTable;
}

bool LibraryCharacteristics::isCombCell(const std::string &name){
  const auto *cell = LibraryParser::get().getLibrary().getCell(name);
  bool funcVerify = !(getFunctions(name).empty()) && areIdsInExpr(getFunctions(name).at(0), getInputs(name));

  auto outputs = getOutputs(name);
  return !cell->hasAttribute("ff") &&
         !cell->hasAttribute("latch") &&
         std::find(outputs.begin(), outputs.end(), "CLK") == outputs.end() &&
         funcVerify &&
         outputs.size() == 1;
}

float LibraryCharacteristics::getArea(const std::string &name) {
  const auto *cell = LibraryParser::get().getLibrary().getCell(name);
  return cell ?
         cell->getFloatAttribute("area", FLT_MAX) : FLT_MAX;
}

float LibraryCharacteristics::getLeakagePower(const std::string &name) {
  const auto *cell = LibraryParser::get().getLibrary().getCell(name);
  return cell ?
         cell->getFloatAttribute("cell_leakage_power", FLT_MAX) : FLT_MAX;
}

std::vector<LibraryCharacteristics::Delay> LibraryCharacteristics::getDelay(
    const std::string &name,
    const std::vector<float> &inputTransTime,
    const float outputCap) {
  std::vector<LibraryCharacteristics::Delay> delay;
  auto ins = getInputs(name);
  for (size_t i = 0; i < ins.size(); i++) {
    delay.push_back(getDelay(name,
                             ins.at(i),
                             inputTransTime.at(i),
                             outputCap));
  }
  return delay;
}

float LibraryCharacteristics::getLutValue(const LookupTable *lut,
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

float LibraryCharacteristics::getTwoAxisLutInterValue(
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

float LibraryCharacteristics::getLutInterValue(
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

float LibraryCharacteristics::getValue(const LookupTable* lut,
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

LibraryCharacteristics::Delay LibraryCharacteristics::getDelay(
    const std::string &name,
    const std::string &relPin,
    const float inputTransTime,
    const float outputCap) {
  LibraryCharacteristics::Delay delay;
  std::vector<float> searchParams = {inputTransTime,outputCap};
  const auto *cell = LibraryParser::get().getLibrary().getCell(name);
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


bool LibraryCharacteristics::isIsolateCell(const std::string &name) {
  return LibraryParser::get().getLibrary().getCell(name)->
      getBooleanAttribute("is_isolation_cell", false);
}

} // namespace eda::gate::library
