//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/check_lib/aig/check_lib_for_aig.h"
#include "gate/library/liberty_manager.h"
#include "gate/library/library.h"
#include "gate/model/utils/subnet_truth_table.h"
#include "gate/optimizer/synthesis/isop.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace eda::gate::library {

using CellSymbol = model::CellSymbol;
using CellType = model::CellType;
using Subnet = model::Subnet;
using Link = Subnet::Link;
using MinatoMorrealeAlg = optimizer::synthesis::MMSynthesizer;
using NetID = model::NetID;
using SubnetBuilder = model::SubnetBuilder;
using SubnetID = model::SubnetID;

std::string exprToString(const Expr *expr);

std::string binOpToString(const Expr *lhs, const std::string &op, const Expr *rhs) {
  return "(" + exprToString(lhs) + op + exprToString(rhs) + ")";
}

std::string exprToString(const Expr *expr) {
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

SCLibrary::SCLibrary() {
  try {
    auto& library = LibertyManager::get().getLibrary();
    for (const auto &cell : library.getCells()) {
      addCell(cell);
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  std::cout << "The number of extracted from Liberty CellTypes is " <<
      combCells.size() << std::endl; // TODO

  assert(isFunctionallyComplete());
}

std::vector<SubnetID> SCLibrary::getSubnetID(const kitty::dynamic_truth_table& tt) const {
  auto it = truthTables.find(tt);
  return (it != truthTables.end()) ? it->second : std::vector<SubnetID>{};
}

const SCAttrs &SCLibrary::getCellAttrs(const SubnetID id) const {
  assert(attributes.find(id) != attributes.end());
  return attributes.at(id);
}

std::vector<SubnetID> &SCLibrary::calcPatterns() {
  assert(attributes.size() != 0);
  if (patterns.size() == 0) {
    for (const auto &pair: attributes) {
      patterns.push_back(pair.first);
    }
  }
  return patterns;
}

inline kitty::dynamic_truth_table create_not(unsigned num_vars) {
  kitty::dynamic_truth_table tt(num_vars);
  kitty::create_nth_var(tt, 0);
  return ~tt;
}

inline kitty::dynamic_truth_table create_and(unsigned num_vars) {
  kitty::dynamic_truth_table tt(num_vars);
  kitty::create_from_binary_string(tt, "0001");
  return tt;
}

inline kitty::dynamic_truth_table create_or(unsigned num_vars) {
  kitty::dynamic_truth_table tt(num_vars);
  kitty::create_from_binary_string(tt, "0111");
  return tt;
}

inline kitty::dynamic_truth_table create_nand(unsigned num_vars) {
  return ~create_and(num_vars);
}

inline kitty::dynamic_truth_table create_nor(unsigned num_vars) {
  return ~create_or(num_vars);
}

inline kitty::dynamic_truth_table create_xor(unsigned num_vars) {
  kitty::dynamic_truth_table tt(num_vars);
  kitty::create_from_binary_string(tt, "0110");
  return tt;
}

inline kitty::dynamic_truth_table create_xnor(unsigned num_vars) {
  return ~create_xor(num_vars);
}

inline bool areIdsInExpr(
              const std::string& expr, const std::vector<std::string> &ids) {
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

bool SCLibrary::isFunctionallyComplete() {
  unsigned num_vars = 2;

  auto tt_not = create_not(1);
  auto tt_and = create_and(num_vars);
  auto tt_or = create_or(num_vars);
  auto tt_nand = create_nand(num_vars);
  auto tt_nor = create_nor(num_vars);
  auto tt_xor = create_xor(num_vars);
  auto tt_xnor = create_xnor(num_vars);

  bool hasNot = truthTables.count(tt_not) > 0;
  bool hasAnd = truthTables.count(tt_and) > 0;
  bool hasOr = truthTables.count(tt_or) > 0;
  bool hasNand = truthTables.count(tt_nand) > 0;
  bool hasNor = truthTables.count(tt_nor) > 0;
  bool hasXor = truthTables.count(tt_xor) > 0;
  bool hasXnor = truthTables.count(tt_xnor) > 0;

  bool hasNonTruePreserving = hasNot || hasNand || hasNor || hasXor || hasXnor;
  bool hasNonFalsePreserving = hasAnd || hasOr || hasNand || hasNor || hasXor || hasXnor;
  bool hasNonMonotonic = hasXor || hasNand || hasNor || hasXnor;
  bool hasNonSelfDual = hasAnd || hasOr || hasNand || hasNor || hasNot || hasXor || hasXnor;

  if (hasNonTruePreserving && hasNonFalsePreserving && hasNonMonotonic && hasNonSelfDual) {
    return true;
  }

  if ((hasAnd && hasOr && hasNot) || hasNand || hasNor) {
    return true;
  }
  return false;
}

bool SCLibrary::isCombCell(
       const Cell &cell,
       const std::vector<std::string> &inputs,
       const std::vector<std::string> &outputs,
       const std::vector<std::string> &funcs) const {
  bool funcVerify = !funcs.empty() && areIdsInExpr(funcs.at(0), inputs);

  return !cell.hasAttribute("ff") &&
         !cell.hasAttribute("latch") &&
         std::find(outputs.begin(), outputs.end(), "CLK") == outputs.end() &&
         funcVerify &&
         outputs.size() == 1;
}

void SCLibrary::calcPower(CellTypeID cellTypeID, std::vector<SCPinPower> &power) {
  const auto &cellType = CellType::get(cellTypeID);
  const auto *cell =
    LibertyManager::get().getLibrary().getCell(cellType.getName());

  for (const auto &pin: (*cell).getPins()) {
    if (pin.getIntegerAttribute("direction", 10) & (1 << 1)) {
      for (const auto &inPwr: pin.getInternalPowerGroups()) {
        float fallPower, risePower;
        bool isFallPower = true;
        for (const auto &lut: inPwr.getLuts()) {
          float sum = 0;
          for (const auto &it: lut.getValues()) {
            sum += it;
          }
          float med = sum / lut.getValuesSize();
          isFallPower ? fallPower = med : risePower = med;
          isFallPower = false;
        }
        power.push_back(SCPinPower{fallPower, risePower});
      }
    }
  }
}

void SCLibrary::addCombCell(
       const std::string &name,
       const std::vector<std::string> &inputs,
       const std::string &func,
       model::CellTypeAttrID cellTypeAttrID) {
  model::CellProperties props(true,false,true, false, false, false,
                              false, false, false);

  kitty::dynamic_truth_table truthTable(inputs.size());
  kitty::create_from_formula(truthTable, func, inputs);

  MinatoMorrealeAlg minatoMorrealeAlg;
  const auto subnetID = minatoMorrealeAlg.synthesize(truthTable);
  const auto cellID = model::makeCellType(
      model::CellSymbol::UNDEF, name, subnetID,
      cellTypeAttrID, props,
      static_cast<uint16_t>(inputs.size()),
      static_cast<uint16_t>(1)
  );
  std::vector<SCPinPower> power;
  calcPower(cellID, power);
  calcPermutations(cellID, power);
  combCells.push_back(cellID);
}

void SCLibrary::addCell(const Cell &cell) {
  if (cell.getBooleanAttribute("is_isolation_cell", false)) {
    return; // TODO
  }

  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  std::vector<std::string> funcs;

  for (const auto& pin : cell.getPins()) {
    if (pin.getIntegerAttribute("direction", 10) & (1 << 0)) {
      inputs.push_back(std::string(pin.getName()));
    }
    if (pin.getIntegerAttribute("direction", 10) & (1 << 1)) {
      outputs.push_back(std::string(pin.getName()));
      if (pin.hasAttribute("function")) {
        const auto* func = pin.getBexprAttribute("function");
        if (func != nullptr) {
          funcs.push_back(exprToString(func));
        }
      }
    }
  }

  if (inputs.empty() || !cell.hasAttribute("area")) {
    return;
  }

  const auto typeAttr = model::makeCellTypeAttr();
  model::CellTypeAttr::get(typeAttr).props.area =
      cell.getFloatAttribute("area", MAXFLOAT);

  if (isCombCell(cell, inputs, outputs, funcs)) {
    addCombCell(std::string(cell.getName()), inputs, funcs.at(0), typeAttr);
  } else {
    //addSeqCell(cell, std::string(cell.getName()), inputs, funcs.at(0), typeAttr);
  }
}

void SCLibrary::calcPermutations(
       CellTypeID cellTypeID, std::vector<SCPinPower> &power) { // TODO substitute by NPN base
  auto &cellType = CellType::get(cellTypeID);
  std::vector<int> permutationVec(cellType.getInNum());
  std::iota(permutationVec.begin(), permutationVec.end(), 0);
  do {
    SubnetBuilder builder;
    SubnetBuilder builder2;

    std::vector<uint16_t> inputs(cellType.getInNum());
    std::vector<uint16_t> inputs2(cellType.getInNum());

    for (std::size_t i = 0; i < cellType.getInNum(); ++i) {
      auto input  = builder.addInput();
      auto input2 = builder2.addInput();

      inputs[permutationVec.at(i)] = input.idx;
      inputs2[permutationVec.at(i)] = input2.idx;
    }

    std::vector<Link> links;
    std::vector<Link> links2;

    for (std::size_t i = 0; i < cellType.getInNum(); ++i) {
      links.emplace_back(inputs[i]);
      links2.emplace_back(inputs2[i]);
    }

    const auto cell = builder.addCell(cellTypeID, links);
    const auto cell2 = builder2.addSubnet(cellType.getImpl(), links);

    builder.addOutput(cell);
    builder2.addOutput(cell2.at(0));

    const auto subnetID = builder.make();
    subnets.push_back(subnetID);

    std::vector<SCPinPower> powers;
    for (size_t i = 0; i < cellType.getInNum(); ++i) {
      powers.push_back(power.at(permutationVec.at(i)));
    }
    SCAttrs attrs{cellType.getName(), cellType.getAttr().props.area, powers};
    attributes[subnetID] = attrs;

    truthTables[model::evaluate(Subnet::get(builder2.make())).at(0)].
      push_back(subnetID);

  } while (std::next_permutation(permutationVec.begin(), permutationVec.end()));
}
} // namespace eda::gate::library
