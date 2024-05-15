//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/optimizer/synthesis/isop.h"
#include "gate/techmapper/library/cell.h"
#include "gate/techmapper/library/liberty_manager.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>
#include <readcells/token_parser.h>
#include <regex>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using std::filesystem::exists;
using std::filesystem::path;
using std::getline;
using std::ifstream;

namespace eda::gate::techmapper {

using MinatoMorrealeAlg = eda::gate::optimizer::synthesis::MMSynthesizer;
using SubnetBuilder = eda::gate::model::SubnetBuilder;
using NetID = eda::gate::model::NetID;


std::string exprToString(const Expr* expr);

std::string binOpToString(const Expr* lhs, const std::string& op, const Expr* rhs) {
  return "(" + exprToString(lhs) + op + exprToString(rhs) + ")";
}

std::string exprToString(const Expr* expr) {
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

bool areAllIdentifiersInVector(const std::string& expression, const std::vector<std::string> &identifiers) {
  std::set<std::string> uniqueIdentifiers(identifiers.begin(), identifiers.end());
  std::regex wordRegex(R"(\b[a-zA-Z0-9]+\b)");
  std::sregex_iterator next(expression.begin(), expression.end(), wordRegex);
  std::sregex_iterator end;
  while (next != end) {
    std::smatch match = *next;
    std::string token = match.str();
    if (uniqueIdentifiers.find(token) == uniqueIdentifiers.end()) {
      return false;
    }
    next++;
  }

  return true;
}

void LibraryCells::readLibertyFile(std::vector<CellTypeID> &cellTypeIDs,
                                   std::vector<CellTypeID> &cellTypeFFIDs,
                                   std::vector<CellTypeID> &cellTypeFFrsIDs,
                                   std::vector<CellTypeID> &cellTypeLatchIDs) {
//  Library lib;
//  try {
//    lib = LibraryManager::get().getLibrary();
//  } catch (const std::exception &e) {
//    std::cerr << "Error: " << e.what() << std::endl;
//  }

  for (const auto& cell : LibraryManager::get().getLibrary().getCells()) {
    auto name = std::string(cell.getName());

    std::vector<std::string> inputs;
    std::vector<std::string> outputs;

    std::vector<std::string> funcs;

    for (const auto &pin : cell.getPins()) {
      if (pin.getIntegerAttribute("direction", 10) & (1 << 0)) {
        inputs.push_back(std::string(pin.getName()));
      }
      if (pin.getIntegerAttribute("direction", 10) & (1 << 1)) {
          outputs.push_back(std::string(pin.getName()));
        if (pin.hasAttribute("function")) {
          const auto *func = pin.getBexprAttribute("function");

          if (func != nullptr) {
            funcs.push_back(exprToString(func));
          }
        }
      }
    }

    if (inputs.empty()) continue;

    if (!cell.hasAttribute("area")) continue;

    model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
    model::CellTypeAttr::get(cellTypeAttrID).props.area =
        cell.getFloatAttribute("area", MAXFLOAT);

    bool funcVerify = false;
    if (!funcs.empty()) {
      funcVerify = areAllIdentifiersInVector(funcs.at(0), inputs);
    }

    if (!cell.hasAttribute("ff") &&
        !cell.hasAttribute("latch") &&
        std::find(outputs.begin(), outputs.end(),"CLK") == outputs.end() &&
        funcVerify &&
        outputs.size() == 1) {

      eda::gate::model::CellProperties
          props(true, false, true, false, false, false, false, false, false);

      kitty::dynamic_truth_table *truthTable =
          new kitty::dynamic_truth_table(inputs.size());
      kitty::create_from_formula(*truthTable, funcs.at(0), inputs);

      MinatoMorrealeAlg minatoMorrealeAlg;
      const auto subnetID = minatoMorrealeAlg.synthesize(*truthTable);

      CellTypeID cellID = eda::gate::model::makeCellType(
          eda::gate::model::CellSymbol::UNDEF,
          name, subnetID,
          cellTypeAttrID, props,
          static_cast<uint16_t>(inputs.size()),
          static_cast<uint16_t>(1));

      cellTypeIDs.push_back(cellID);
    } // TODO: add sequence cells
  }
/*
      } else {
        if (it.value().contains("ff") && it.value()["ff"].get<bool>()) {
          if (inputPinNames.size() == 2) {
            eda::gate::model::CellProperties
                props(false, false, false, false, false, false, false, false,
                      false);

            model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
            model::CellTypeAttr::get(cellTypeAttrID).props.area = it.value()["area"];

            SubnetBuilder subnetBuilder;
            auto in1 = subnetBuilder.addInput();
            auto in2 = subnetBuilder.addInput();
            auto dff = subnetBuilder.addCell(model::CellSymbol::DFF, in2, in1);
            subnetBuilder.addOutput(dff);

            CellTypeID cellID = eda::gate::model::makeCellType(
                eda::gate::model::CellSymbol::UNDEF,
                it.key(), subnetBuilder.make(),
                cellTypeAttrID, props,
                static_cast<uint16_t>(inputPinNames.size()),
                static_cast<uint16_t>(it.value()["output"].size()));

            cellTypeFFIDs.push_back(cellID);
          }
        } else if (it.value().contains("ffrs") && it.value()["ffrs"].get<bool>()) {
          if (inputPinNames.size() == 4) {
            eda::gate::model::CellProperties
                props(false, false, false, false, false, false, false, false,
                      false);

            model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
            model::CellTypeAttr::get(cellTypeAttrID).props.area = it.value()["area"];

            SubnetBuilder subnetBuilder;
            auto in1 = subnetBuilder.addInput();
            auto in2 = subnetBuilder.addInput();
            auto in3 = subnetBuilder.addInput();
            auto in4 = subnetBuilder.addInput();
            auto dffrs = subnetBuilder.addCell(model::CellSymbol::DFFrs, in2, in1, in3, in4);
            subnetBuilder.addOutput(dffrs);

            CellTypeID cellID = eda::gate::model::makeCellType(
                eda::gate::model::CellSymbol::UNDEF,
                it.key(), subnetBuilder.make(),
                cellTypeAttrID, props,
                static_cast<uint16_t>(inputPinNames.size()),
                static_cast<uint16_t>(it.value()["output"].size()));

            cellTypeFFrsIDs.push_back(cellID);
          }

        } else if (it.value().contains("latch") && it.value()["latch"].get<bool>()) {
          if (inputPinNames.size() == 2) {
            eda::gate::model::CellProperties
                props(false, false, false, false, false, false, false, false,
                      false);

            model::CellTypeAttrID cellTypeAttrID = model::makeCellTypeAttr();
            model::CellTypeAttr::get(cellTypeAttrID).props.area = it.value()["area"];

            SubnetBuilder subnetBuilder;
            auto in1 = subnetBuilder.addInput();
            auto in2 = subnetBuilder.addInput();
            auto latch = subnetBuilder.addCell(model::CellSymbol::LATCH, in1, in2);
            subnetBuilder.addOutput(latch);

            CellTypeID cellID = eda::gate::model::makeCellType(
                eda::gate::model::CellSymbol::UNDEF,
                it.key(), subnetBuilder.make(),
                cellTypeAttrID, props,
                static_cast<uint16_t>(inputPinNames.size()),
                static_cast<uint16_t>(it.value()["output"].size()));

            cellTypeLatchIDs.push_back(cellID);
          }
        }
      }
    }
  }*/
}
} // namespace eda::gate::techmapper
