//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library.h"
#include "gate/optimizer/synthesis/isop.h"

#include <readcells/ast.h>
#include <readcells/ast_parser.h>

namespace eda::gate::library {

using CellTypeID = model::CellTypeID;

SCLibrary::SCLibrary(const std::string &fileName) {
  // TODO: if we want to avoid usage FILE, we need to fix it in ReadCells.
  FILE *file = fopen(fileName.c_str(), "rb");
  if (file != nullptr) {
    ast = tokParser.parseLibrary(file, fileName.c_str());

    AstParser parser(library, tokParser);
    parser.run(*ast);
    fclose(file);

    iface = new ReadCellsIface(library);

    for (const auto &name : iface->getCells()) {
      if (iface->isIsolationCell(name)) {
        continue;
      }

      if (iface->isCombCell(name)) {
        loadCombCell(name);
      } else {
        //loadSeqCell(cell);
      }
    }

    findCheapestCells();
    addSuperCells();
    addConstCells();
  }
}

SCLibrary::~SCLibrary() {
  if (iface != nullptr) {
    delete iface;
  }
}

void SCLibrary::loadCombCell(const std::string &name) {
  const auto ports = iface->getPorts(name);

  const auto nInputs = model::CellTypeAttr::getInBitWidth(ports);
  const auto nOutputs = model::CellTypeAttr::getOutBitWidth(ports);
  const auto props = iface->getPhysProps(name);
  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(props);
  model::SubnetID subnetID;

  // We can't consider cells with no area.
  if (props.area == MAXFLOAT || nOutputs == 0) {
    return;
  }

  model::SubnetBuilder builder;
  std::vector<kitty::dynamic_truth_table> funcs;
  if (nInputs == 0) {
    std::vector<std::string> varNames;
    for (uint out = 0; out < nOutputs; out++) {
      std::string strFunc = iface->getStringFunction(name, out);
      if (strFunc == "0" || strFunc == "1") {
        const auto cell = builder.addCell(strFunc == "0" ? model::ZERO : model::ONE);
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
      kitty::dynamic_truth_table func {iface->getFunction(name, out)};
      funcs.push_back(func);
      auto subnetObject = optimizer::synthesis::MMSynthesizer{}.synthesize(func);
      assert(subnetObject.hasBuilder());
      auto &funcBuilder = subnetObject.builder();
      const auto funcID = funcBuilder.make();

      const auto outputs = builder.addSubnet(funcID, inputs);
      builder.addOutputs(outputs);
    }
  }
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
    name,
    subnetID,
    attrID,
    model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
    nInputs,
    nOutputs);

  if (nInputs > maxArity) {
    maxArity = nInputs;
  }

  std::vector<kitty::dynamic_truth_table> ctt;
  std::vector<util::NpnTransformation> t;
  for (auto func : funcs) {
    auto config = kitty::exact_p_canonization(func);
    ctt.push_back(util::getTT(config)); // canonized TT
    t.push_back(util::getTransformation(config));
  }

  StandardCell cell{cellTypeID, ctt, t};
  for (auto func : funcs) {
    const auto strFunc = kitty::to_hex(func);
    if (nInputs == 0) {
      if (strFunc == "0") {
        constZeroCells.push_back(cell);
      } else if (strFunc == "1") {
        constOneCells.push_back(cell);
      }
    } else {
      if (nInputs == 1 && strFunc == "1") {
        negCombCells.push_back(cell);
      }
    }
  }
  combCells.push_back(cell);
}

const SCLibrary::StandardCell *SCLibrary::findCheapestCell(
                                 std::vector<StandardCell> &scs) {
  float lowArea = MAXFLOAT;
  float lowPower = MAXFLOAT;
  const SCLibrary::StandardCell *result = nullptr;
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
  if (negCombCells.empty()) {
    assert(false && "Neg cell is not found in Liberty file!");
    // TODO add hand-made negative combinational cell
  }
  if (constOneCells.empty()) {
    assert(false && "Const One is not found in Liberty file!");
    // TODO add hand-made const-one cell
  }
  if (constZeroCells.empty()) {
    assert(false && "Const Zero is not found in Liberty file!");
    // TODO add hand-made const-zero cell
  }

  // Find the cheapest negation and constant cells.
  cheapNegCell = findCheapestCell(negCombCells);
  cheapOneCell = findCheapestCell(constOneCells);
  cheapZeroCell = findCheapestCell(constZeroCells);
}

inline void replaceSubStr(std::string &in, const std::string &subStr,
                                           const std::string &newSubStr) {
  size_t pos = 0;
  while ((pos = in.find(subStr, pos)) != std::string::npos) {
    in.replace(pos, subStr.length(), newSubStr);
    pos += newSubStr.length();
  }
}

void inverseFirstInput(std::string &in, const std::string &fIn) {
  std::string invFirstInput = "!(" + fIn + ")";
  replaceSubStr(in, fIn, invFirstInput);
}

void highFirstInput(std::string &in, const std::string &fIn) {
  std::string one = "(1)";
  replaceSubStr(in, fIn, one);
}

void lowFirstInput(std::string &in, const std::string &fIn) {
  std::string zero = "(0)";
  replaceSubStr(in, fIn, zero);
}

void SCLibrary::addSuperCell(
    const model::CellTypeID cellTypeID,
    const model::CellTypeID cellTypeIDToAdd,
    void(*updateFunc)(std::string &in, const std::string &fIn),
    std::vector<StandardCell> &scs,
    uint output) {
  const auto &cellType = model::CellType::get(cellTypeID);
  const auto &cellTypeToAdd = model::CellType::get(cellTypeIDToAdd);

  const auto &addedProps = cellTypeToAdd.getAttr().getPhysProps();
  auto newProps = iface->getPhysProps(cellType.getName());
  newProps.area += addedProps.area;
  newProps.delay += addedProps.delay;
  newProps.power += addedProps.power;

  const auto ports = iface->getPorts(cellType.getName());
  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(newProps);

  // Construction of SubnetBuilder for two cells: the cell "cellToAdd" is
  // connected to the first input of the other cell.
  model::SubnetBuilder builder;
  // Two LinkLists: for cellToAdd and for the cell.
  model::SubnetBuilder::LinkList inputLinks[2];

  size_t i = 0;
  // First create inputs for the closer-to-inputs cell.
  if (cellTypeToAdd.getInNum() == 0) {
    builder.addInput(); // TODO: Fantom input
    i++;
  } else {
    for (; i < cellTypeToAdd.getInNum(); i++) {
      const auto input = builder.addInput();
      inputLinks[0].push_back(input);
    }
  }
  // Then create inputs for the second cell.
  for (; i < cellType.getInNum(); i++) {
    const auto input = builder.addInput();
    inputLinks[1].push_back(input);
  }

  model::Subnet::Link cellToAdd;
  if (cellTypeToAdd.getOutNum() > 1) {
    const auto outputs =
      builder.addMultiOutputCell(cellTypeIDToAdd, inputLinks[0]);
    cellToAdd = outputs[output];
  } else {
    cellToAdd = builder.addCell(cellTypeIDToAdd, inputLinks[0]);
  }

  inputLinks[1].insert(inputLinks[1].begin(), cellToAdd);
  const auto outputs = builder.addMultiOutputCell(cellTypeID, inputLinks[1]);
  builder.addOutputs(outputs);

  const auto subnetID = builder.make();
#if DEBUG_MOUTS
  if (cellType.getOutNum() > 1)
    std::cout << "Added 2-out Super Cell: " << model::Subnet::get(subnetID) << std::endl;
#endif

  // Obtain new CellTypeID for the supercell.
  const auto cellTypeIDsc = model::makeCellType(
    model::CellSymbol::UNDEF,
    cellType.getName() + "*" + cellTypeToAdd.getName(),
    subnetID,
    attrID,
    model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
    cellType.getInNum(),
    cellType.getOutNum());

  std::vector<kitty::dynamic_truth_table> ctt;
  std::vector<util::NpnTransformation> t;
  // Calculate new truth table for the supercell.
  for (uint i = 0; i < cellType.getOutNum(); i++) {
    auto strFunc = iface->getStringFunction(cellType.getName(), i);
    const auto inputs = iface->getInputs(cellType.getName());
    updateFunc(strFunc, inputs[0]);

    kitty::dynamic_truth_table tt(inputs.size());
    kitty::create_from_formula(tt, strFunc, inputs);

    auto config = kitty::exact_p_canonization(tt);
    ctt.push_back(util::getTT(config)); // canonized TT
    t.push_back(util::getTransformation(config));
  }
  scs.push_back({cellTypeIDsc, ctt, t});
}

void SCLibrary::addSuperCells() {
  std::vector<StandardCell> superCells;

  // find all two-input elements
  // inverse left input and add this element
  for (const auto &cell : combCells) {
    const auto &cellType = model::CellType::get(cell.cellTypeID);
    if (cellType.getInNum() == 2) {
      addSuperCell(cell.cellTypeID, cheapNegCell->cellTypeID,
        inverseFirstInput, superCells, 0);

      uint output = 0;
      for (const auto& ctt : cheapOneCell->ctt) {
        if (kitty::to_hex(ctt) == "1") {
          break;  
        }
        output++;
      }
      assert(output < model::CellType::get(
                        cheapOneCell->cellTypeID).getOutNum());
      addSuperCell(cell.cellTypeID, cheapOneCell->cellTypeID,
        highFirstInput, superCells, output);

      output = 0;
      for (const auto& ctt : cheapZeroCell->ctt) {
        if (kitty::to_hex(ctt) == "0") {
          break;
        }
        output++;
      }
      assert(output < model::CellType::get(
                        cheapZeroCell->cellTypeID).getOutNum());

      addSuperCell(cell.cellTypeID, cheapZeroCell->cellTypeID,
        lowFirstInput, superCells, output);
    }
  }
  combCells.insert(combCells.end(), superCells.begin(), superCells.end());
}

void SCLibrary::addConstCells() {
  combCells.insert(
    combCells.end(), constOneCells.begin(), constOneCells.end());
  combCells.insert(
    combCells.end(), constZeroCells.begin(), constZeroCells.end());
}

SCLibrary *library = nullptr;

} // namespace eda::gate::library
