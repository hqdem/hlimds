//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/readcells_srcfile_parser.h"
#include <readcells/ast.h>
#include <readcells/ast_parser.h>

//questionable includes for fullExtractOldWay
#include "gate/optimizer/synthesis/isop.h"
#include <cfloat>
namespace eda::gate::library {

using FilePtr = std::unique_ptr<FILE, decltype(std::fclose) *>;
using RCGetGroupPtr = const std::pmr::list<readcells::AttributeList*>*;
//conversion functions between pmr and regular vector
// Conversion that copies all entries (const lvalue vector)
template <typename NewAllocator, typename T, typename OldAllocator>
std::vector<T, NewAllocator> convert_vector(
    const std::vector<T, OldAllocator>& v) {
  auto result = std::vector<T, NewAllocator>{};
  result.reserve(v.size());
  result.assign(v.begin(), v.end());
  return result;
}
// conversion that moves all entries (rvalue vector)
template <typename NewAllocator, typename T, typename OldAllocator>
std::vector<T, NewAllocator> convert_vector(
    std::vector<T, OldAllocator>&& v) {
  auto result = std::vector<T, NewAllocator>{};
  result.reserve(v.size());
  result.assign(
    std::make_move_iterator(v.begin()), 
    std::make_move_iterator(v.end())
  );
  return result;
}

std::string exprToString(const readcells::Expr *expr);

ReadCellsParser::ReadCellsParser(std::string filename) {
  // TODO: if we want to avoid usage FILE, we need to fix it in ReadCells.
  FilePtr file(fopen(filename.c_str(), "rb"), std::fclose);
  if (file != nullptr) {
    readcells::Group *ast = tokParser_.parseLibrary(file.get(),
                                                    filename.c_str());

    readcells::AstParser parser(library_, tokParser_);
    parser.run(*ast);
  } else {
    throw std::runtime_error("Failed to open file: " + filename);
  }
}

static bool isIsolationCell(const readcells::AttributeList &cell) {
  return cell.getBooleanAttribute("is_isolation_cell", false);
}

std::vector<StandardCell> ReadCellsParser::extractCells() {
  std::vector<StandardCell> extractedCells;

  const RCGetGroupPtr cellGroup = library_.getGroup("cell");
  if (cellGroup == nullptr) {
    return extractedCells;
  }
  for (const auto *rcCell : *cellGroup) {
    if (rcCell == nullptr || isIsolationCell(*rcCell)) {
      continue;
    }

    if (isCombCell(*rcCell)) {
      if (StandardCell standardCell; loadCombCell(standardCell, *rcCell)) {
        extractedCells.push_back(std::move(standardCell));
      }
    } else {
      //loadSeqCell(cell);
    }
  }
  return extractedCells;
}

std::vector<WireLoadModel> ReadCellsParser::extractWLMs() {
  std::vector<WireLoadModel> wlms;
  if (RCGetGroupPtr wlmGroup = library_.getGroup("wire_load");
      wlmGroup != nullptr) {
    for (const auto *model : *wlmGroup) {
      WireLoadModel wlm;
      wlm.name = model->getName();
      wlm.resistance = model->getFloatAttribute("resistance", 0);
      wlm.capacitance = model->getFloatAttribute("capacitance", 0);
      wlm.slope = model->getFloatAttribute("slope", 0);
      
      if (const auto *fanoutList = model->getComplexAttrs("fanout_length");
        fanoutList != nullptr) {
        for (const auto *elem : *fanoutList) {
          WireLoadModel::FanoutLength fanoutLength {
            static_cast<size_t>(elem->values[0].ival), elem->values[1].fval};
          wlm.wireLength.push_back(fanoutLength);
        }
        std::reverse(wlm.wireLength.begin(), wlm.wireLength.end());
      }
      wlms.push_back(wlm);
    };
  }
  return wlms;
}

CSFProperties ReadCellsParser::extractProperties() {
  CSFProperties props;
  props.defaultWLM = std::string(library_.getStringAttribute("default_wire_load", ""));
  if (RCGetGroupPtr wlSelection = library_.getGroup("wire_load_selection");
      wlSelection != nullptr) {
    for (const auto *elem : *wlSelection) {
      if (const auto *origWLFromArea = elem->getComplexAttrs("wire_load_from_area");
          origWLFromArea != nullptr) {
        for (const auto *member : *origWLFromArea) {
          WireLoadSelection::WireLoadFromArea wlFromArea;
          wlFromArea.leftBound = member->values[0].ival;
          wlFromArea.rightBound = member->values[1].ival;
          wlFromArea.wlmName = member->values[2].sval;
          props.WLSelection.wlmFromArea.push_back(wlFromArea);
        }
      }
    }
  }
  std::reverse(props.WLSelection.wlmFromArea.begin(), props.WLSelection.wlmFromArea.end());
  return props;
}

std::vector<LutTemplate> ReadCellsParser::extractTemplates() {
  std::vector<LutTemplate> templates;
  for (const auto *luTempl : library_.getTemplates()) {
    LutTemplate t;
    t.name = luTempl->getName();
    for (const auto &index : *luTempl) {
      t.variables.push_back(LutTemplate::NameID(index.id));
      t.indexes.push_back(
          convert_vector<std::allocator<double>>(index.values));
    }
    templates.push_back(std::move(t));
  }
  return templates;
}

static inline long getDirection(const readcells::AttributeList &pin) {
  return (pin.getIntegerAttribute("direction", readcells::Pin::Dir_None));
}

//create internal Lut object from ReadCells object
static library::LUT transformLut(const readcells::LookupTable &rcLut) {
  //TODO: properly parse all relevant information
  library::LUT newLut;
  for (const auto &index : rcLut) {
      newLut.indexes.push_back(
        convert_vector<std::allocator<double>>(index.values));
  }
  newLut.values = convert_vector<std::allocator<double>>(rcLut.getValues());
  return newLut;
}
//returns true if lut with specified name was found and added to receiver
template <typename LutProvider, typename lutReciever>
static inline bool setLut(lutReciever &receiver,
                          const char* name,
                          const LutProvider &provider) {
  if (const auto *lut = provider.getLastLut(name); lut != nullptr) {
    receiver = transformLut(*lut);
    return true;
  }
  return false;
}

//returns true if lut with specified name was found and added to receiver
template <typename LutProvider, typename lutReciever>
static inline bool setLut(std::vector<lutReciever> &receiver,
                          const char* name,
                          const LutProvider &provider) {
  if (const auto *lut = provider.getLastLut(name); lut != nullptr) {
    receiver.push_back(transformLut(*lut));
    return true;
  }
  return false;
}

static bool parsePinCommonPart(const readcells::AttributeList &pin,
                               library::Pin &commonPin) {
  commonPin.name = pin.getName();
  const RCGetGroupPtr ipGroup = pin.getGroup("internal_power");
  if (ipGroup != nullptr) {
    for (const auto *swPwr : *ipGroup) {
      setLut(commonPin.powerFall, "fall_power", *swPwr);
      setLut(commonPin.powerRise, "rise_power", *swPwr);
    }
  }
  return true;
}

static bool parseInputPin(const readcells::AttributeList &rcPin,
                         std::vector<InputPin> &inputPins) {
  InputPin in;
  parsePinCommonPart(rcPin, in);
  in.capacitance = rcPin.getFloatAttribute("capacitance", 0);
  in.fallCapacitance = rcPin.getFloatAttribute("fall_capacitance", 0);
  in.riseCapacitance = rcPin.getFloatAttribute("rise_capacitance", 0);
  inputPins.push_back(std::move(in));
  return true;
}

static std::string getOutputPinStringFunction(
    const readcells::AttributeList &pin) {
  const readcells::Expr *func = pin.getBexprAttribute("function");
  std::string result;
  if (func != nullptr) {
    result = exprToString(func);
  }
  return result;
}

static bool parseOutputPin(const readcells::AttributeList &rcPin,
                          std::vector<OutputPin> &outputPins) {
  OutputPin out;
  parsePinCommonPart(rcPin, out);
  out.maxCapacitance = rcPin.getFloatAttribute("max_capacitance", 0);
  out.stringFunction = getOutputPinStringFunction(rcPin);

  //extracting Timing LUTs
  const RCGetGroupPtr timingGroup = rcPin.getGroup("timing");
  if (timingGroup != nullptr) {
    for (const auto *timing : *timingGroup) {
      setLut(out.delayFall, "cell_fall", *timing);
      setLut(out.delayRise, "cell_rise", *timing);
      setLut(out.slewFall, "fall_transition", *timing);
      setLut(out.slewRise, "rise_transition", *timing);
      out.timingSence.push_back(
        timing->getIntegerAttribute("timing_sense", 0));
    }
  }

  outputPins.push_back(std::move(out));
  return true;
}

void ReadCellsParser::setCellPins(StandardCell& standardCell,
                                  const readcells::AttributeList &rcCell) {
  const RCGetGroupPtr pinGroup = rcCell.getGroup("pin");
  if (pinGroup != nullptr) {
    for (const auto *pin : *pinGroup) {
      if (pin == nullptr) {
        continue;
      }
      auto direction = getDirection(*pin);
      switch(direction) {
        case readcells::Pin::Dir_Input : {
          parseInputPin(*pin, standardCell.inputPins);
          break;
        }
        case readcells::Pin::Dir_Output : {
          parseOutputPin(*pin, standardCell.outputPins);
          break;
        }
        default : {
          //TODO: handle error
        }
      }
    }
  }
}

std::string binOpToString(
    const readcells::Expr *expr, const std::string &op);

std::string exprToString(const readcells::Expr *expr) {
  if (!expr) return "";

  std::stringstream ss;

  switch (expr->kind) {
    case readcells::EK_Identifier:
      ss << expr->name;
      break;
    case readcells::EK_Literal:
      ss << (expr->bval ? "1" : "0");
      break;
    case readcells::EK_Subscript:
      ss << expr->name << "[" << exprToString(expr->opnd) << "]";
      break;
    case readcells::EK_Not:
      ss << "!(" << exprToString(expr->opnd) << ")";
      break;
    case readcells::EK_Xor:
      ss << binOpToString(expr, "^");
      break;
    case readcells::EK_And:
      ss << binOpToString(expr, "&");
      break;
    case readcells::EK_Or:
      ss << binOpToString(expr, "|");
      break;
    default:
      ss << "unknown";
      break;
  }
  return ss.str();
}

std::string binOpToString(
    const readcells::Expr *expr, const std::string &op) {
  return "(" + exprToString(expr->binop.lhs) + op +
               exprToString(expr->binop.rhs) + ")";
}

void getIdsInExpr(std::map<std::string, int> &ids,
                  const readcells::Expr *expr) {
  if (expr->kind == readcells::EK_Identifier) {
    ids[std::string(expr->name)] = 1;
  } else if (expr->kind == readcells::EK_Not ||
             expr->kind == readcells::EK_Subscript) {
    getIdsInExpr(ids, expr->opnd);
  } else if (expr->kind == readcells::EK_Xor ||
             expr->kind == readcells::EK_And ||
             expr->kind == readcells::EK_Or) {
    getIdsInExpr(ids, expr->binop.lhs);
    getIdsInExpr(ids, expr->binop.rhs);
  }
}

bool checkIdsInExpr(const readcells::Expr *expr,
                    const std::vector<std::string> &ids) {
  std::map<std::string, int> existingIds;
  getIdsInExpr(existingIds, expr);
  for (const auto &id : ids) {
    if (existingIds.find(id) == existingIds.end()) {
      return false;
    }
  }
  return true;
}

inline bool isInputPin(const readcells::AttributeList &pin) {
  return getDirection(pin) == readcells::Pin::Dir_Input;
}

inline bool isOutputPin(const readcells::AttributeList &pin) {
  return getDirection(pin) == readcells::Pin::Dir_Output;
}

const readcells::AttributeList* ReadCellsParser::getOutputPin(
    const readcells::AttributeList &rcCell, uint number) {
  const readcells::AttributeList *output = nullptr;
  uint currentPin = 0;
  const RCGetGroupPtr pinGroup = rcCell.getGroup("pin");
  if (pinGroup != nullptr) {
    for (const auto *pin : *pinGroup) {
      if (pin == nullptr) {
        continue;
      }
      if (isOutputPin(*pin) && (currentPin++ == number)) {
        output = pin;
        break;
      }
    }
  }
  return output;
}

const readcells::Expr *ReadCellsParser::getExprFunction(
    const readcells::AttributeList &rcCell, uint number) {
  const auto *pin = getOutputPin(rcCell, number);
  const readcells::Expr *func = nullptr;
  if (pin != nullptr) {
    func = pin->getBexprAttribute("function");
  }
  return func;
}

std::vector<std::string> ReadCellsParser::getInputPinNames(
    const readcells::AttributeList &rcCell) {
  std::vector<std::string> inputs;
  const RCGetGroupPtr pinGroup = rcCell.getGroup("pin");
  if (pinGroup != nullptr) {
    for (const auto *pin : *pinGroup) {
      if (pin == nullptr) {
        continue;
      }
      if (isInputPin(*pin)) {
        inputs.push_back(std::string(pin->getName()));
      }
    }
  }
  return inputs;
}

std::vector<std::string> ReadCellsParser::getOutputPinNames(
    const readcells::AttributeList &rcCell) {
  std::vector<std::string> outputs;
  const RCGetGroupPtr pinGroup = rcCell.getGroup("pin");
  if (pinGroup != nullptr) {
    for (const auto *pin : *pinGroup) {
      if (pin == nullptr) {
        continue;
      }
      if (isOutputPin(*pin)) {
        outputs.push_back(std::string(pin->getName()));
      }
    }
  }
  return outputs;
}

//TODO: looks to complicated
bool ReadCellsParser::isCombCell(const readcells::AttributeList &rcCell){
  const auto &inputPinNames = getInputPinNames(rcCell);
  const auto &outputPinNames = getOutputPinNames(rcCell);
  const auto* firsOutPinExpr = getExprFunction(rcCell, 0);
  bool funcVerify = (inputPinNames.size() == 0 && outputPinNames.size() != 0) ||
                    ((firsOutPinExpr != nullptr) &&
                    checkIdsInExpr(firsOutPinExpr, inputPinNames));

  return !rcCell.hasAttribute("ff") &&
         !rcCell.hasAttribute("latch") &&
         std::find(outputPinNames.begin(), outputPinNames.end(), "CLK")
                  == outputPinNames.end() &&
         funcVerify &&
         outputPinNames.size() > 0/*== 1*/;
}

double ReadCellsParser::getArea(const readcells::AttributeList &rcCell) {
  return rcCell.getFloatAttribute("area", NAN);
}

double ReadCellsParser::getLeakagePower(
    const readcells::AttributeList &rcCell) {
  return rcCell.getFloatAttribute("cell_leakage_power", NAN);
}

void ReadCellsParser::setCellProperties(
    StandardCell& standardCell,
    const readcells::AttributeList &rcCell) {
  standardCell.propertyLeakagePower = getLeakagePower(rcCell);
  standardCell.propertyArea = getArea(rcCell);
  //TODO: need to calculate some meaningfull value here
  standardCell.propertyDelay = 1.;
}

bool ReadCellsParser::loadCombCell(StandardCell& standardCell,
                                   const readcells::AttributeList &rcCell) {
  standardCell.name = rcCell.getName();
  setCellPins(standardCell, rcCell);
  setCellProperties(standardCell, rcCell);
  return true;
}

} // namespace eda::gate::library