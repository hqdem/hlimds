//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library.h"
#include "gate/optimizer/synthesis/isop.h"

namespace eda::gate::library {

using CellTypeID = model::CellTypeID;

bool SCLibrary::checkCellCollisions(const std::vector<StandardCell> &cells) {
  for (const auto &cell : cells) {
    if (auto res = collisions_.cellNames.insert(cell.name);
        res.second == false) {
      throw std::runtime_error(
        std::string("Cell name collision for: ") + cell.name);
    return true;
    }
  }
  return false;
}

bool SCLibrary::checkTemplateCollisions(const std::vector<LutTemplate> &templates) {
  for (const auto &tmpl : templates) {
    if (auto res = collisions_.templateNames.insert(tmpl.name);
        res.second == false) {
      throw std::runtime_error(
        std::string("Template name collision for: ") + tmpl.name);
      return true;
    }
  }
  return false;
}

bool SCLibrary::checkWLMCollisions(const std::vector<WireLoadModel> &wlms) {
  for (const auto &wlm : wlms) {
    if (auto res = collisions_.wlmNames.insert(wlm.name);
        res.second == false) {
      throw std::runtime_error(
        std::string("WLM name collision for: ") + wlm.name);
      return true;
    }
  }
  return false;
}

bool SCLibrary::addCells(std::vector<StandardCell> &&cells) {
  for (auto &cell : cells) {
    if (cell.inputPins.size() < 8) { // FIXME: kitty's pcanonization doesn't support in>7
      internalLoadCombCell(std::move(cell));
    }
  }
  return true;
}

bool SCLibrary::addTemplates(std::vector<LutTemplate> &&templates) {
  if (templates_.empty()) {
    templates_ = std::move(templates);
  } else {
    templates_.insert(templates_.end(), templates.begin(), templates.end());
  }
  return true;
}

bool SCLibrary::addWLMs(std::vector<WireLoadModel> &&wlms) {
  if (wires_.empty()) {
    wires_ = std::move(wlms);
  } else {
    wires_.insert(wires_.end(), wlms.begin(), wlms.end());
  }
  return true;
}

bool SCLibrary::addProperties(const std::string &defaultWLMsName,
                              WireLoadSelection &&selection) {
  for (const auto &wlm : wires_) {
    if (wlm.name == defaultWLMsName) {
      properties_.defaultWLM = &wlm;
      break;
    }
  }
  properties_.wlmSelection = std::move(selection);
  return true;
}

void SCLibrary::fillSearchMap() {
  for (const auto& cell : combCells_) {
    searchMap_[cell.cellTypeID] = &cell;
  }
}

const StandardCell* SCLibrary::getCellPtr(
    const model::CellTypeID &cellTypeID) const {
  if (auto search = searchMap_.find(cellTypeID); search != searchMap_.end()) {
    const StandardCell* cell = search->second;
    return cell;
  }
  return nullptr;
}

static model::CellType::PortVector getPorts(const StandardCell &cell) {
  model::CellType::PortVector ports;
  size_t index{0};

  for (const auto &pin : cell.inputPins) {
    //args: name, width, isInput, index
    ports.emplace_back(pin.name, 1, true, index++);
  }
  for (const auto &pin : cell.outputPins) {
    //args: name, width, isInput, index
    ports.emplace_back(pin.name, 1, false, index++);
  }
  return ports;
}

static model::PhysicalProperties getPhysProps(const StandardCell &cell) {
  model::PhysicalProperties props;
  props.area = cell.propertyArea;
  props.delay = cell.propertyDelay;
  props.power = cell.propertyLeakagePower;
  return props;
}

static std::vector<std::string> getInputNames(
    const std::vector<InputPin> &inputPins) {
  std::vector<std::string> names;
  for (const auto &pin : inputPins) {
    names.push_back(pin.name);
  }
  return names;
}

static kitty::dynamic_truth_table getFunction(
    const std::string &stringFunction,
    const std::vector<InputPin> &inputPins) {
  const auto &inputNames = getInputNames(inputPins); // TODO: assert inputs.size() != 0
  kitty::dynamic_truth_table truthTable(inputPins.size());
  kitty::create_from_formula(truthTable,
    stringFunction, inputNames);
  return truthTable;
}

void SCLibrary::internalLoadCombCell(StandardCell &&cell) {
  const auto ports = getPorts(cell);
  const auto nInputs = model::CellTypeAttr::getInBitWidth(ports);
  const auto nOutputs = model::CellTypeAttr::getOutBitWidth(ports);
  const auto props = getPhysProps(cell);

  assert(nInputs == cell.inputPins.size());
  assert(nOutputs == cell.outputPins.size());

  // We can't consider cells with no area.
  if (std::isnan(props.area) || nOutputs == 0) {
    return;
  }

  model::SubnetBuilder builder;
  std::vector<kitty::dynamic_truth_table> funcs;
  if (nInputs == 0) {
    std::vector<std::string> varNames;
    for (uint out = 0; out < nOutputs; out++) {
      std::string strFunc = cell.outputPins[out].stringFunction;
      if (strFunc == "0" || strFunc == "1") {
        const auto cell = builder.addCell(
                                    strFunc == "0" ? model::ZERO : model::ONE);
        builder.addOutput(cell);
        kitty::dynamic_truth_table func;
        kitty::create_from_formula(func, strFunc, varNames);
        funcs.push_back(func);
      } else {
        return;
      }
    }
  } else {
    model::SubnetBuilder::LinkList inputs(nInputs);
    for (uint in = 0; in < nInputs; in++) {
      inputs[in] = builder.addInput();
    }
    for (uint out = 0; out < nOutputs; out++) {
      kitty::dynamic_truth_table func {
        getFunction(cell.outputPins[out].stringFunction, cell.inputPins)};
      funcs.push_back(func);
      auto subnetObject =
        optimizer::synthesis::MMSynthesizer{}.synthesize(func);
      assert(subnetObject.hasBuilder());
      auto &funcBuilder = subnetObject.builder();
      const auto funcID = funcBuilder.make();

      const auto outputs = builder.addSubnet(funcID, inputs);
      builder.addOutputs(outputs);
    }
  }

  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(props);
  model::SubnetID subnetID;

  subnetID = builder.make();
#if DEBUG_MOUTS
  if (nInputs == 0)
    std::cout << "added no input cell: " << model::Subnet::get(subnetID);

  if (name == "sky130_fd_sc_hd__ha_1") {
    std::cout << name << std::endl;
    auto tt2 = model::evaluate(model::Subnet::get(subnetID));
    std::cout << model::Subnet::get(subnetID) << std::endl <<
      kitty::to_hex(tt2[0]) << std::endl <<
      kitty::to_hex(tt2[1]) << std::endl;
  }
#endif

  const auto cellTypeID = model::makeCellType(
    model::CellSymbol::UNDEF,
    cell.name,
    subnetID,
    attrID,
    model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
    nInputs,
    nOutputs);

  if (nInputs > properties_.maxArity) {
    properties_.maxArity = nInputs;
  }

  std::vector<kitty::dynamic_truth_table> ctt;
  std::vector<util::NpnTransformation> t;
  for (auto func : funcs) {
    auto config = kitty::exact_p_canonization(func);
    ctt.push_back(util::getTT(config)); // canonized TT
    t.push_back(util::getTransformation(config));
  }

  //TODO: probaly better to fill this during parsing
  cell.cellTypeID = cellTypeID;
  cell.ctt = ctt;
  cell.transform = t;

  for (auto tt : cell.ctt) {
    const auto strFunc = kitty::to_hex(tt);
    if (nInputs == 0) {
      if (strFunc == "0") {
        constZeroCells_.push_back(cell);
      } else if (strFunc == "1") {
        constOneCells_.push_back(cell);
      }
    } else {
      if (nInputs == 1 && strFunc == "1") {
        negCombCells_.push_back(cell);
      }
    }
  }
  combCells_.push_back(std::move(cell));
}

const StandardCell *SCLibrary::findCheapestCell (
    const std::vector<StandardCell> &scs) const {
  float lowArea = MAXFLOAT;
  float lowPower = MAXFLOAT;
  const StandardCell *result = nullptr;
  for (const auto &cell : scs) {
    const auto &cellType = model::CellType::get(cell.cellTypeID);
    const auto &props = cellType.getAttr().getPhysProps();
    if ((props.area < lowArea) ||
        (props.area == lowArea && props.power < lowPower)) {
      lowArea = props.area;
      lowPower = props.power;
      result = &cell;
    }
  }
  return result;
}

void SCLibrary::findCheapestCells() {
  if (negCombCells_.empty()) {
    assert(false && "Neg cell is not found in Liberty file!");
    // TODO add hand-made negative combinational cell
  }
  if (constOneCells_.empty()) {
    assert(false && "Const One is not found in Liberty file!");
    // TODO add hand-made const-one cell
  }
  if (constZeroCells_.empty()) {
    assert(false && "Const Zero is not found in Liberty file!");
    // TODO add hand-made const-zero cell
  }

  // Find the cheapest negation and constant cells.
  properties_.cheapNegCell = findCheapestCell(negCombCells_);
  properties_.cheapOneCell = findCheapestCell(constOneCells_);
  properties_.cheapZeroCell = findCheapestCell(constZeroCells_);
}

static model::CellTypeAttrID createSuperCellPropertiesAttr(
    const StandardCell &cellSrc,
    const StandardCell &cellToAdd) {
  model::PhysicalProperties newProps;
  newProps.area = cellSrc.propertyArea + cellToAdd.propertyArea;
  newProps.delay = cellSrc.propertyDelay + cellToAdd.propertyDelay;
  newProps.power = cellSrc.propertyLeakagePower + cellToAdd.propertyLeakagePower;

  const auto ports = getPorts(cellSrc); //TODO: it is bad to recalculate it again
  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(newProps);
  return attrID;
}

static gate::model::SubnetID buildSuperCellSubnet(
    const StandardCell &cellSrc,
    const StandardCell &cellToAdd,
    uint output) {
  model::SubnetBuilder builder;
  // Two LinkLists: for cellToAdd and for the cell.
  model::SubnetBuilder::LinkList inputLinks[2];

  size_t i = 0;
  size_t ouputCntOfAdd = cellToAdd.outputPins.size();
  size_t inputCntOfAdd = cellToAdd.inputPins.size();
  size_t inputCntOfSrc = cellSrc.inputPins.size();
  // First create inputs for the closer-to-inputs cell.
  if (inputCntOfAdd == 0) {
    builder.addInput(); // TODO: Fantom input
    i++;
  } else {
    for (; i < inputCntOfAdd; i++) {
      const auto input = builder.addInput();
      inputLinks[0].push_back(input);
    }
  }
  // Then create inputs for the second cell.
  for (; i < inputCntOfSrc; i++) {
    const auto input = builder.addInput();
    inputLinks[1].push_back(input);
  }

  model::Subnet::Link cellToAddLink;
  if (ouputCntOfAdd > 1) {
    const auto outputs =
      builder.addMultiOutputCell(cellToAdd.cellTypeID, inputLinks[0]);
    cellToAddLink = outputs[output];
  } else {
    cellToAddLink = builder.addCell(cellToAdd.cellTypeID, inputLinks[0]);
  }

  inputLinks[1].insert(inputLinks[1].begin(), cellToAddLink);
  const auto outputs = builder.addMultiOutputCell(cellSrc.cellTypeID,
                                                  inputLinks[1]);
  builder.addOutputs(outputs);

  const auto subnetID = builder.make();
  return subnetID;
}

void SCLibrary::addSuperCell(
    const StandardCell &cellSrc,
    const StandardCell &cellToAdd,
    std::vector<StandardCell> &scs,
    uint output) {

  const auto &cellType = model::CellType::get(cellSrc.cellTypeID);
  const auto &cellTypeToAdd = model::CellType::get(cellToAdd.cellTypeID);

  const auto attrID = createSuperCellPropertiesAttr(cellSrc, cellToAdd);

  // Construction of SubnetBuilder for two cells: the cell "cellToAdd" is
  // connected to the first input of the other cell.
  const auto subnetID = buildSuperCellSubnet(cellSrc, cellToAdd, output);

  // Obtain new CellTypeID for the supercell.
  const auto superCellTypeID = model::makeCellType(
    model::CellSymbol::UNDEF,
    cellType.getName() + "*" + cellTypeToAdd.getName(),
    subnetID,
    attrID,
    model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
    cellType.getInNum(), // TODO it may be reduced
    cellType.getOutNum());

  std::vector<kitty::dynamic_truth_table> ctt;
  std::vector<util::NpnTransformation> t;
  // Calculate new truth table for the supercell.
  const auto tt = model::evaluate(model::Subnet::get(subnetID));
  for (uint i = 0; i < cellType.getOutNum(); i++) {
    auto config = kitty::exact_p_canonization(tt[i]);
    ctt.push_back(util::getTT(config)); // canonized TT
    t.push_back(util::getTransformation(config));
  }

#ifdef DEBUG_MOUTS
  if (cellType.getOutNum() > 1) {
    std::cout << "\nAdded 2-out Super Cell: " << model::Subnet::get(subnetID);
    uint ttn = 0;
    for (const auto &tte : tt) {
      std::cout << "Its tt[" << ttn++ << "]=" << kitty::to_hex(tte) << std::endl;
    }
    ttn = 0;
    for (const auto &tte : cellSrc.ctt) {
      std::cout << "Orig cell tt[" << ttn++ << "]=" << kitty::to_hex(tte) << std::endl;
    }
    ttn = 0;
    const auto &subnet = cellTypeToAdd.getSubnet();
    const auto ttna = model::evaluate(subnet);
    for(const auto &tte : ttna) {
      std::cout << "Added cell tt[" << ttn++ << "]=" << kitty::to_hex(tte) << std::endl;
    }
  }
#endif

  //TODO: newly created supercell should have all fields properly filled
  StandardCell newSuperCell = cellSrc;
  newSuperCell.cellTypeID = superCellTypeID;
  newSuperCell.ctt = ctt;
  newSuperCell.transform = t;
  newSuperCell.name = cellType.getName() + "*" + cellTypeToAdd.getName();
  //TODO FIXME: properly fill other newSuperCell fields
  scs.push_back(newSuperCell);
}

void SCLibrary::addSuperCells() {
  std::vector<StandardCell> superCells;

  // find all two-input elements
  // inverse left input and add this element
  for (const auto &cell : combCells_) {
    if (cell.inputPins.size() != 2) {
      continue;
    }
    addSuperCell(cell, *properties_.cheapNegCell, superCells, 0);

    uint output = 0;
    for (const auto& ctt : properties_.cheapOneCell->ctt) {
      if (kitty::to_hex(ctt) == "1") {
        break;
      }
      output++;
    }
    assert(output < model::CellType::get(
                      properties_.cheapOneCell->cellTypeID).getOutNum());
    addSuperCell(cell, *properties_.cheapOneCell,superCells, output);

    output = 0;
    for (const auto& ctt : properties_.cheapZeroCell->ctt) {
      if (kitty::to_hex(ctt) == "0") {
        break;
      }
      output++;
    }
    assert(output < model::CellType::get(
                      properties_.cheapZeroCell->cellTypeID).getOutNum());
    addSuperCell(cell, *properties_.cheapZeroCell, superCells, output);
  }
  combCells_.insert(combCells_.end(), superCells.begin(), superCells.end());
}

} // namespace eda::gate::library
