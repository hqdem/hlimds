//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/library/library.h"
#include "gate/optimizer/synthesis/isop.h"

#include <bitset>
#include <set>
namespace eda::gate::library {

using CellTypeID = model::CellTypeID;
using CanonInfo = SCLibrary::CanonInfo;

void SCLibrary::updateProperties() {
  for (const auto& cell: combCells_) {
    size_t inputNum = cell.inputPins.size();
    if (inputNum > properties_.maxArity) {
      properties_.maxArity = inputNum;
    }
  }
}

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

//TODO: probably should be templated by num_vars
template<size_t NUMVAR>
static CanonInfo getCanonF(uint64_t functionBinRep) {
  std::string binaryString = std::bitset< 64 >(functionBinRep).to_string();
  std::reverse(binaryString.begin(), binaryString.end());
  binaryString.resize(1<<NUMVAR);// kitty wants string to have exact number of characters
  std::reverse(binaryString.begin(), binaryString.end());
  model::TruthTable tt(NUMVAR);
  kitty::create_from_binary_string(tt, binaryString);
  auto epc = kitty::exact_p_canonization(tt);
  auto canonTt = util::getTT(epc);
  auto transform = util::getTransformation(epc);
  return {canonTt, transform};
}

static std::set<model::TruthTable> generateP2classes() {
  std::set<model::TruthTable> eqClasses;
  for (size_t functionBinRep = 0; functionBinRep < 16; ++functionBinRep) {
    auto canon = getCanonF<2>(functionBinRep);
    eqClasses.insert(canon.ctt);
  }
  return eqClasses;
}

static std::set<model::TruthTable> generateP3classes() {
  std::set<model::TruthTable> eqClasses;
  for (size_t functionBinRep = 0; functionBinRep < 256; ++functionBinRep) {
    auto canon = getCanonF<3>(functionBinRep);
    eqClasses.insert(canon.ctt);
  }
  return eqClasses;
}

void SCLibrary::completePclasses() {
  //complete for 3
  CttMap p1Map;
  CttMap p2Map;
  CttMap p3Map;
  for (const auto& cell : combCells_) {
    uint16_t output = 0;
    for (const auto& ctt : cell.ctt) {
      if (ctt._num_vars == 1) {
        p1Map[ctt].push_back(std::pair{&cell, output++});
      } else if (ctt._num_vars == 2) {
        p2Map[ctt].push_back(std::pair{&cell, output++});
      } else if (ctt._num_vars == 3) {
        p3Map[ctt].push_back(std::pair{&cell, output++});
      }
    }
  }
  // check that p1 space is complete
  if (p1Map.size() != 2) {
    //assert(false);
    completeP1classes(p1Map);
  }
  // check that p2 space is complete
  if (p2Map.size() != 12) {
    //assert(false);
    completeP2classes(p2Map);
  }
  // check that p3 space is complete
  if (p3Map.size() != 80) {
    completeP3classes(p3Map, p2Map);
  }
  // store newly generated cells as comb cells
  combCells_.insert(combCells_.end(), pComplCells_.begin(), pComplCells_.end());
  pComplCells_.clear();
}

void SCLibrary::completeP1classes(CttMap &existingCttP1) {
  assert(properties_.cheapNegCell.first != nullptr);
  const auto &cellInv = properties_.cheapNegCell;
  //if we got in here we are missing buffer. Lets connect two invertors just for fun
  uint8_t functionBinRep = 0x2;
  auto canon = getCanonF<1>(functionBinRep);
  auto bufCell = addNegOutput(cellInv, cellInv);
  assert(canon.ctt == bufCell.first->ctt[0]);
  existingCttP1[canon.ctt].push_back(bufCell);
}

static std::vector<uint8_t> getMiniTerms(uint64_t funcBinRep, size_t funcLength = 0) {
  std::vector<uint8_t> miniTerms;
  miniTerms.reserve(funcLength); //no more than 8 miniterms for 3 variable function
  size_t i = 0;
  while (funcBinRep) {
    if (funcBinRep & 1) {
      miniTerms.push_back(i);
    }
    funcBinRep = funcBinRep >> 1;
    i++;
  }
  return miniTerms;
}

// │ B │ A │
// │ 0 │ 0 │
// │ 0 │ 1 │
// │ 1 │ 0 │
// │ 1 │ 1 │
static std::vector<CanonInfo> getF2CanonRep(
    const std::vector<uint8_t> &miniTerms) {

  std::vector<CanonInfo> canons;
  canons.reserve(miniTerms.size());
  for (const auto term : miniTerms) {
    uint8_t functionBinRep = 0xF;
    functionBinRep &= (term & 1) ? 0xA : ~0xA;
    functionBinRep &= (term & 2) ? 0xC : ~0xC;
    auto canon = getCanonF<2>(functionBinRep);
    canons.push_back(canon);
  }
  return canons;
}

static model::CellTypeAttrID createP1PropertiesAttr(
    const StandardCell &cellSrc,
    const StandardCell &cellNeg) {
  model::PhysicalProperties newProps;
  newProps.area = cellSrc.propertyArea + cellNeg.propertyArea;
  newProps.delay = cellSrc.propertyDelay + cellNeg.propertyDelay;
  newProps.power = cellSrc.propertyLeakagePower +
                  cellNeg.propertyLeakagePower;

  //const auto ports = getPorts(cellSrc);
  model::CellType::PortVector ports;
  size_t index{0};

  //hardcode input pin
  ports.emplace_back("INPUT_A", 1, true, index++);
  //register output pin
  ports.emplace_back("OUTPUT_Y", 1, false, index++);
  //TODO: this might break
  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(newProps);
  return attrID;
}

static model::CellTypeAttrID createP2F2PropertiesAttr(
    const StandardCell &cellAnd,
    const StandardCell &cellNeg,
    size_t mult) {
  model::PhysicalProperties newProps;
  newProps.area = cellAnd.propertyArea + cellNeg.propertyArea * mult;
  newProps.delay = cellAnd.propertyDelay + cellNeg.propertyDelay;
  newProps.power = cellAnd.propertyLeakagePower +
                  cellNeg.propertyLeakagePower * mult;

  //const auto ports = getPorts(cellSrc);
  model::CellType::PortVector ports;
  size_t index{0};

  //hardcode 2 inputs for 2 variable function
  ports.emplace_back("INPUT_A", 1, true, index++);
  ports.emplace_back("INPUT_B", 1, true, index++);
  //register output pin
  ports.emplace_back("OUTPUT_Y", 1, false, index++);
  //TODO: this might break
  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(newProps);
  return attrID;
}

SCLibrary::CellLogPair SCLibrary::addNegOutput(const CellLogPair &sourceCell,
                                               const CellLogPair &exCellInv) {
  const auto& cellSrc = *sourceCell.first;
  const auto cellSrcOutput = sourceCell.second;
  const auto& cellInv = *exCellInv.first;
  const auto cellInvOutput = exCellInv.second;
  size_t fanoutSrc = cellSrc.outputPins.size();

  model::SubnetBuilder builder;
  model::SubnetBuilder::LinkList inputLinks;

  for (size_t i = 0; i < cellSrc.inputPins.size(); ++i) {
    inputLinks.push_back(builder.addInput());
  }

  model::SubnetBuilder::LinkList permLinks(inputLinks.size());
  for (size_t i = 0; i < cellSrc.transform[cellSrcOutput].permutation.size(); ++i) {
    auto permId = cellSrc.transform[cellSrcOutput].permutation[i];
    permLinks[i] = inputLinks[permId];
  }

  model::Subnet::LinkList srcOutLink;
  if (fanoutSrc > 1) {
    const auto outputs =
      builder.addMultiOutputCell(cellSrc.cellTypeID, inputLinks);
    srcOutLink.push_back(outputs[cellSrcOutput]);
  } else {
    srcOutLink.push_back(builder.addCell(cellSrc.cellTypeID, inputLinks));
  }


  size_t fanoutInv = cellInv.outputPins.size();
  if (fanoutInv > 1) {
    const auto outputs =
      builder.addMultiOutputCell(cellInv.cellTypeID, srcOutLink);
    builder.addOutput(outputs[cellInvOutput]);
  } else {
    builder.addOutput(builder.addCell(cellInv.cellTypeID, srcOutLink));
  }
  const auto subnetID = builder.make();

  //std::cout << model::Subnet::get(subnetID) << std::endl;
//---------------
  //register subnet
  //FIXME: this assumes pin assignments inside
  const auto attrID = createP1PropertiesAttr(cellSrc, cellInv);
  std::string p2CellName = cellSrc.name + "*->*" + cellInv.name;
  const auto p2CellTypeID = model::makeCellType(
    model::CellSymbol::UNDEF,
    p2CellName,
    subnetID,
    attrID,
    model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
    cellSrc.inputPins.size(), // TODO this might be wrong
    1);

  // create new comb cell
  std::vector<kitty::dynamic_truth_table> ctt;
  std::vector<util::NpnTransformation> t;

  const auto &p2CellType = model::CellType::get(p2CellTypeID);
  // Calculate new truth table for the cell.
  const auto tt = model::evaluate(model::Subnet::get(subnetID));
  for (uint i = 0; i < p2CellType.getOutNum(); i++) {
    auto config = kitty::exact_p_canonization(tt[i]);
    ctt.push_back(util::getTT(config)); // canonized TT
    t.push_back(util::getTransformation(config));
  }
  //TODO: newly created supercell should have all fields properly filled
  StandardCell newSuperCell = cellSrc;
  newSuperCell.cellTypeID = p2CellTypeID;
  newSuperCell.ctt = ctt;
  newSuperCell.transform = t;
  newSuperCell.name = p2CellName;
  //TODO FIXME: properly fill other newSuperCell fields
  newSuperCell.propertyArea = cellSrc.propertyArea + cellInv.propertyArea;
  newSuperCell.propertyDelay = cellSrc.propertyDelay + cellInv.propertyDelay;
  newSuperCell.propertyLeakagePower = cellSrc.propertyLeakagePower +
                                      cellInv.propertyLeakagePower;

  pComplCells_.push_back(newSuperCell);
  //FIXME: correct ctt is not allways at 0 pos
  return {&pComplCells_.back(), 0};
}

SCLibrary::CellLogPair
SCLibrary::getBaseP2Term(CttMap &existingCttP2, const CellLogPair &exCellInv) {
  uint8_t termFuncBinRep[] = {8,4,2,1}; //reverse order to find better solitions first
  CanonInfo canon;
  for (auto func : termFuncBinRep) {
    const auto canon = getCanonF<2>(func);
    if (existingCttP2.count(canon.ctt)) {
      return existingCttP2[canon.ctt][0];
    }
  }
  // try to find negation of term and attach inverter to its output
  uint8_t termNegFuncBinRep[] = {7,11,13,14};
  for (auto func : termNegFuncBinRep) {
    const auto canon = getCanonF<2>(func);
    if (existingCttP2.count(canon.ctt)) {
      const auto negTerm = existingCttP2[canon.ctt][0];
      return addNegOutput(negTerm, exCellInv);
    }
  }
  //CellLogPair res = createP2SingleTerm();
  assert(false && "No easy way to generate AND and OR in P2");
  return {};
}

std::pair <SCLibrary::CellLogPair, SCLibrary::CellLogPair>
SCLibrary::createP2AndOR(CttMap &existingCttP2, const CellLogPair &exCellInv) {
  CellLogPair andCell, orCell, termCell;
  uint8_t andFunc = 0x8, andNegFunc = 0x7;
  uint8_t orFunc = 0xE, orNegFunc = 0x1;
  const auto canonAnd = getCanonF<2>(andFunc);
  const auto canonAndNeg = getCanonF<2>(andNegFunc);
  const auto canonOr = getCanonF<2>(orFunc);
  const auto canonOrNeg = getCanonF<2>(orNegFunc);
  if (existingCttP2.count(canonAnd.ctt)) {
    andCell = existingCttP2[canonAnd.ctt][0];
  } else if (existingCttP2.count(canonAndNeg.ctt)) {
    andCell = addNegOutput(existingCttP2[canonAndNeg.ctt][0], exCellInv);
    existingCttP2[andCell.first->ctt[andCell.second]].push_back(andCell);
  } else {
    termCell = getBaseP2Term(existingCttP2, exCellInv);
    uint64_t funcBits = termCell.first->ctt[termCell.second]._bits[0];
    auto terms = getMiniTerms(funcBits, 4);
    andCell = buildP2CellForF2MiniTerm(terms[0], termCell, exCellInv);
    existingCttP2[termCell.first->ctt[termCell.second]].push_back(termCell);
    existingCttP2[andCell.first->ctt[andCell.second]].push_back(andCell);
  }
  if (existingCttP2.count(canonOr.ctt)) {
    orCell = existingCttP2[canonOr.ctt][0];
  } else if (existingCttP2.count(canonOrNeg.ctt)) {
    orCell = addNegOutput(existingCttP2[canonOrNeg.ctt][0], exCellInv);
    existingCttP2[andCell.first->ctt[andCell.second]].push_back(andCell);
  } else {
    if (termCell.first == nullptr) {
      termCell = getBaseP2Term(existingCttP2, exCellInv);
      existingCttP2[termCell.first->ctt[termCell.second]].push_back(termCell);
    }
    uint64_t funcBits = termCell.first->ctt[termCell.second]._bits[0];
    auto terms = getMiniTerms(funcBits, 4);
    terms[0] = terms[0] ^ 3;
    auto negOrCell = buildP2CellForF2MiniTerm(terms[0], termCell, exCellInv);
    orCell = addNegOutput(negOrCell, exCellInv);
    existingCttP2[negOrCell.first->ctt[negOrCell.second]].push_back(negOrCell);
    existingCttP2[orCell.first->ctt[orCell.second]].push_back(orCell);
  }
  return {andCell, orCell};
}

SCLibrary::CellLogPair SCLibrary::buildP2CellForF2MiniTerm(
    uint8_t miniTermF2, 
    const CellLogPair &cellAndP,
    const CellLogPair &cellInvP) {

//Connect cells in subnet
  model::SubnetBuilder builder;
  model::SubnetBuilder::LinkList inputLinks;

  inputLinks.push_back(builder.addInput()); //inputA
  inputLinks.push_back(builder.addInput()); //inputB

  const auto& cellNeg = *cellInvP.first;
  const auto cellNegOutput = cellInvP.second;
  const auto& cellAnd = *cellAndP.first;
  const auto cellAndOutput = cellAndP.second;
  size_t fanoutNeg = cellInvP.first->outputPins.size();

  model::Subnet::LinkList afterInvLinks;
  if ((miniTermF2 & 1) == 0) {
    if (fanoutNeg > 1) {
      const auto outputs =
        builder.addMultiOutputCell(cellNeg.cellTypeID, {inputLinks[0]});
      afterInvLinks.push_back(outputs[cellNegOutput]);
    } else {
      afterInvLinks.push_back(builder.addCell(cellNeg.cellTypeID, {inputLinks[0]}));
    }
  } else {
    afterInvLinks.push_back(inputLinks[0]);
  }
  if ((miniTermF2 & 2) == 0) {
    if (fanoutNeg > 1) {
      const auto outputs =
        builder.addMultiOutputCell(cellNeg.cellTypeID, {inputLinks[1]});
      afterInvLinks.push_back(outputs[cellNegOutput]);
    } else {
      afterInvLinks.push_back(builder.addCell(cellNeg.cellTypeID, {inputLinks[1]}));
    }
  } else {
    afterInvLinks.push_back(inputLinks[1]);
  }
  model::SubnetBuilder::LinkList permLinksAnd(2);
  for (size_t i = 0; i < cellAnd.transform[cellAndOutput].permutation.size(); ++i) {
    auto permId = cellAnd.transform[cellAndOutput].permutation[i];
    permLinksAnd[i] = afterInvLinks[permId];
  }

  size_t fanoutAnd = cellAnd.outputPins.size();
  if (fanoutAnd > 1) {
    const auto outputs =
      builder.addMultiOutputCell(cellAnd.cellTypeID, permLinksAnd);
    builder.addOutput(outputs[cellAndOutput]);
  } else {
    builder.addOutput(builder.addCell(cellAnd.cellTypeID, permLinksAnd));
  }
  const auto subnetID = builder.make();

  //std::cout << model::Subnet::get(subnetID) << std::endl;
//---------------
  //register subnet
  //FIXME: this assumes pin assignments inside
  size_t mult = (miniTermF2 & 1) + ((miniTermF2 & 2) >> 1);
  const auto attrID = createP2F2PropertiesAttr(cellAnd, cellNeg, mult);
  std::string p2CellName = cellAnd.name;
  if ((miniTermF2 & 2) == 0) { p2CellName = "NegIn1:" + p2CellName; }
  if ((miniTermF2 & 1) == 0) { p2CellName = "NegIn0:" + p2CellName; }
  const auto p2CellTypeID = model::makeCellType(
    model::CellSymbol::UNDEF,
    p2CellName,
    subnetID,
    attrID,
    model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
    2, // TODO this might be wrong
    1);

  // create new comb cell
  std::vector<kitty::dynamic_truth_table> ctt;
  std::vector<util::NpnTransformation> t;

  const auto &p2CellType = model::CellType::get(p2CellTypeID);
  // Calculate new truth table for the cell.
  const auto tt = model::evaluate(model::Subnet::get(subnetID));
  for (uint i = 0; i < p2CellType.getOutNum(); i++) {
    auto config = kitty::exact_p_canonization(tt[i]);
    ctt.push_back(util::getTT(config)); // canonized TT
    t.push_back(util::getTransformation(config));
  }
  //TODO: newly created supercell should have all fields properly filled
  StandardCell newSuperCell = cellAnd;
  newSuperCell.cellTypeID = p2CellTypeID;
  newSuperCell.ctt = ctt;
  newSuperCell.transform = t;
  newSuperCell.name = p2CellName;
  //TODO FIXME: properly fill other newSuperCell fields
  newSuperCell.propertyArea = cellAnd.propertyArea + cellNeg.propertyArea * mult;
  newSuperCell.propertyDelay = cellAnd.propertyDelay + cellNeg.propertyDelay;
  newSuperCell.propertyLeakagePower = cellAnd.propertyLeakagePower +
                                      cellNeg.propertyLeakagePower * mult;

  pComplCells_.push_back(newSuperCell);
  //FIXME: correct ctt is not allways at 0 pos
  return {&pComplCells_.back(), 0};
}

static model::CellTypeAttrID createF2TermPropertiesAttr(
    const std::vector<std::pair<const StandardCell*,uint16_t>> &cellEquivalents,
    const StandardCell &cellOr) {

  model::PhysicalProperties newProps;
  for (const auto &cellPair : cellEquivalents) {
    const auto &cell = *cellPair.first;
    newProps.area += cell.propertyArea;
    newProps.delay += cell.propertyDelay;
    newProps.power += cell.propertyLeakagePower;
  }
  //const auto ports = getPorts(cellSrc);
  model::CellType::PortVector ports;
  size_t index{0};

  //hardcode 3 inputs for 3 variable function
  ports.emplace_back("INPUT_A", 1, true, index++);
  ports.emplace_back("INPUT_B", 1, true, index++);
  //register output pin
  ports.emplace_back("OUTPUT_Y", 1, false, index++);


  //TODO: this might break
  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(newProps);
  return attrID;
}

model::SubnetBuilder::LinkList connectLayerToORs(
    const model::SubnetBuilder::LinkList &inputLinks,
    model::SubnetBuilder &builder,
    const std::pair<const StandardCell*,uint16_t> exCellOr) {

  model::SubnetBuilder::LinkList newOutputs;

  const auto& cell = *exCellOr.first;
  const auto cellOutput = exCellOr.second;
  size_t fanout = cell.outputPins.size();
  for (size_t i = 0, middle = inputLinks.size() / 2; i < middle; i++) {
    size_t linkPos = i*2;
    model::SubnetBuilder::LinkList permLinks(2);
    for (size_t j = 0; j < cell.transform[cellOutput].permutation.size(); ++j) {
      auto permId = cell.transform[cellOutput].permutation[j];
      permLinks[j] = inputLinks[linkPos + permId];
    }

    model::SubnetBuilder::Link outputLink;
    if (fanout > 1) {
      const auto outputs =
        builder.addMultiOutputCell(cell.cellTypeID, permLinks);
      outputLink = outputs[cellOutput];
    } else {
      outputLink = builder.addCell(cell.cellTypeID, permLinks);
    }
    newOutputs.push_back(outputLink);
  }
  if (inputLinks.size() % 2 != 0) {
    newOutputs.push_back(inputLinks.back());
  }
  return newOutputs;
}

SCLibrary::CellLogPair SCLibrary::buildF2termEquivalentCell(
    const std::vector<CanonInfo> &termCanons,
    const std::vector<CellLogPair > &cellEquivalents,
    const CellLogPair &exCellOr) {

  const auto &cellOr = *exCellOr.first;
  auto cellOrOutput = exCellOr.second;
  // make connections in form of binary tree to reduce delay
  model::SubnetBuilder builder;
  model::SubnetBuilder::LinkList inputLinks;
  inputLinks.push_back(builder.addInput());
  inputLinks.push_back(builder.addInput());

  // connect inputs to miniterm cellEquivalents
  model::SubnetBuilder::LinkList internalOutLinks;
  for (size_t i = 0; i < cellEquivalents.size(); ++i) {
    const auto& cell = *cellEquivalents[i].first;
    const auto cellOutput = cellEquivalents[i].second;
    const auto& term = termCanons[i];

    // term --<perm>-- canon function --<perm>-- StandardCell
    model::SubnetBuilder::LinkList termPermLinks(inputLinks.size());
    size_t termPermSize = term.transform.permutation.size();
    for (size_t i = 0; i < termPermSize; ++i) {
      auto permId = term.transform.permutation[i];
      termPermLinks[i] = inputLinks[permId];
    }
    //TODO: why this reverse is needed?
    std::reverse(termPermLinks.begin(), termPermLinks.end());

    model::SubnetBuilder::LinkList cellPermLinks(termPermLinks.size());
    size_t cellPermSize = cell.transform[cellOutput].permutation.size();
    for (size_t i = 0; i < cellPermSize; ++i) {
      auto permId = cell.transform[cellOutput].permutation[i];
      cellPermLinks[i] = termPermLinks[permId];
    }
    //TODO: why this reverse is needed?
    std::reverse(cellPermLinks.begin(), cellPermLinks.end());

    size_t fanout = cell.outputPins.size();
    model::SubnetBuilder::Link outputLink;
    if (fanout > 1) {
      const auto outputs =
        builder.addMultiOutputCell(cell.cellTypeID, cellPermLinks);
      outputLink = outputs[cellOutput];
    } else {
      outputLink = builder.addCell(cell.cellTypeID, cellPermLinks);
    }
    internalOutLinks.push_back(outputLink);
  }

  // connect pair of terms to OR element. If number of terms is odd,
  // last term will be connected to the las OR element before subnet output
  while (internalOutLinks.size() != 1) {
    internalOutLinks = connectLayerToORs(internalOutLinks, builder, exCellOr);
  }
  // finish creating subnet
  builder.addOutput(internalOutLinks[0]);
  const auto subnetID = builder.make();

  //std::cout << model::Subnet::get(subnetID) << std::endl;

  //register subnet
  const auto attrID = createF2TermPropertiesAttr(
    cellEquivalents, cellOr);
  std::string p2Name = cellEquivalents[0].first->name;
  for (size_t i = 1; i < cellEquivalents.size(); ++i) {
    const auto& cellPair  = cellEquivalents[i];
    p2Name += "*|*" + cellPair.first->name;
  }
  const auto p3CellTypeID = model::makeCellType(
    model::CellSymbol::UNDEF,
    p2Name,
    subnetID,
    attrID,
    model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
    3, // TODO this might be wrong
    1);

  // create new comb cell
  std::vector<kitty::dynamic_truth_table> ctt;
  std::vector<util::NpnTransformation> t;

  const auto &p3CellType = model::CellType::get(p3CellTypeID);
  // Calculate new truth table for the cell.
  const auto tt = model::evaluate(model::Subnet::get(subnetID));
  for (uint i = 0; i < p3CellType.getOutNum(); i++) {
    auto config = kitty::exact_p_canonization(tt[i]);
    ctt.push_back(util::getTT(config)); // canonized TT
    t.push_back(util::getTransformation(config));
  }

  StandardCell newSuperCell;
  newSuperCell.cellTypeID = p3CellTypeID;
  newSuperCell.ctt = ctt;
  newSuperCell.transform = t;
  newSuperCell.name = p2Name;
  //TODO FIXME: properly fill other newSuperCell fields

  newSuperCell.propertyArea = 0;
  newSuperCell.propertyDelay = 0;
  newSuperCell.propertyLeakagePower = 0;
  //FIXME: alot of input ports here, but only 3 in registered subnet ports
  for (const auto &cellPair : cellEquivalents) {
    const auto &cell = *cellPair.first;
    newSuperCell.propertyArea += cell.propertyArea;
    newSuperCell.propertyDelay += cell.propertyDelay;
    newSuperCell.propertyLeakagePower += cell.propertyLeakagePower;
  }
  for (auto pin : cellEquivalents[0].first->inputPins) {
    newSuperCell.inputPins.push_back(pin);
  }
  newSuperCell.outputPins.push_back(cellOr.outputPins[cellOrOutput]);
  pComplCells_.push_back(newSuperCell);

  //FIXME: correct ctt is not allways at 0 pos
  return {&pComplCells_.back(), 0};
}

void SCLibrary::completeP2classes(CttMap &existingCttP2) {
  auto p2Classes = generateP2classes();
  for (const auto &funcClass : existingCttP2) {
    p2Classes.erase(funcClass.first); //erasure is expensive!!!
  }
  // p2Classes now contain all of the missing classes
  // 1. find inv
  assert(properties_.cheapNegCell.first != nullptr);
  const auto &cellInv = properties_.cheapNegCell;
  auto [cellAnd, cellOR] = createP2AndOR(existingCttP2, cellInv);
  for (const auto& func : p2Classes) {
    uint64_t funcBinRep = func._bits[0]; //kinda bad
    // process const 0 and const 1 cases
    if (funcBinRep == 0) {
      continue;
    }
    if (funcBinRep == 0xF) {
      continue;
    }

    // build Sum-of-products canonical form
    std::vector<uint8_t> miniTerms = getMiniTerms(funcBinRep, 4);
    // get canon function class for each miniterm
    std::vector<CanonInfo> termCanonFunc = getF2CanonRep(miniTerms);
    // create subnet representing function from SoPcanonical form
    std::vector<CellLogPair > cellEquivalents;
    for (size_t i = 0, size = miniTerms.size(); i < size; ++i) {
      // some miniterms already exist in p3, get corresponding cells
      if (existingCttP2.count(termCanonFunc[i].ctt)) {
        //TODO: don't allways take first cell with matching ctt, compare them
        cellEquivalents.push_back(existingCttP2[termCanonFunc[i].ctt][0]);
      } else {
        // build p2 representation for each remaining term
        // p2 space should be complete so we are guaranteed to build one
        auto newCellTerm = buildP2CellForF2MiniTerm(miniTerms[i], cellAnd, cellInv);
        const auto &newctCtt = newCellTerm.first->ctt[0];
        assert(newctCtt == termCanonFunc[i].ctt);
        cellEquivalents.push_back(newCellTerm);
        // remember new P3 Ctt function
        existingCttP2[termCanonFunc[i].ctt].push_back(newCellTerm);
      }
    }

    // conect terms with OR to create function
    auto newCell = buildF2termEquivalentCell(termCanonFunc, cellEquivalents, cellOR);
    const auto &newCtt = newCell.first->ctt[0];
    assert(newCtt == func);
    existingCttP2[func].push_back(newCell);
  }
}

// │ C │ B │ A │
// │ 0 │ 0 │ 0 │
// │ 0 │ 0 │ 1 │
// │ 0 │ 1 │ 0 │
// │ 0 │ 1 │ 1 │
// │ 1 │ 0 │ 0 │
// │ 1 │ 0 │ 1 │
// │ 1 │ 1 │ 0 │
// │ 1 │ 1 │ 1 │
static std::vector<CanonInfo> getF3CanonRep(
    const std::vector<uint8_t> &miniTerms) {

  std::vector<CanonInfo> canons;
  canons.reserve(miniTerms.size());
  for (const auto term : miniTerms) {
    uint8_t functionBinRep = 0xFF;
    functionBinRep &= (term & 1) ? 0xAA : ~0xAA;
    functionBinRep &= (term & 2) ? 0xCC : ~0xCC;
    functionBinRep &= (term & 4) ? 0xF0 : ~0xF0;
    auto canon = getCanonF<3>(functionBinRep);
    canons.push_back(canon);
  }
  return canons;
}

static model::CellTypeAttrID createP2F3PropertiesAttr(
    const StandardCell &cellAB,
    const StandardCell &cellDC) {
  model::PhysicalProperties newProps;
  newProps.area = cellAB.propertyArea + cellDC.propertyArea;
  newProps.delay = cellAB.propertyDelay + cellDC.propertyDelay;
  newProps.power = cellAB.propertyLeakagePower + cellDC.propertyLeakagePower;

  //const auto ports = getPorts(cellSrc);
  model::CellType::PortVector ports;
  size_t index{0};

  //hardcode 3 inputs for 3 variable function
  ports.emplace_back("INPUT_A", 1, true, index++);
  ports.emplace_back("INPUT_B", 1, true, index++);
  ports.emplace_back("INPUT_C", 1, true, index++);
  //register output pin
  ports.emplace_back("OUTPUT_Y", 1, false, index++);
  //TODO: this might break
  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(newProps);
  return attrID;
}

// │ C │ B │ A │
// │ 0 │ 0 │ 0 │
// │ 0 │ 0 │ 1 │
// │ 0 │ 1 │ 0 │
// │ 0 │ 1 │ 1 │
// │ 1 │ 0 │ 0 │
// │ 1 │ 0 │ 1 │
// │ 1 │ 1 │ 0 │
// │ 1 │ 1 │ 1 │
SCLibrary::CellLogPair SCLibrary::buildP2CellForF3MiniTerm(
    uint8_t miniTermF3, CttMap &existingCttP2) {

  // decompose miniTermF3 (A&B&C) into 2 P2 and terms (D = A&B; D&C)
  // A connected to input 0, B connected to input 1
  uint8_t funcBinRepAB = 0xF;
  funcBinRepAB &= (miniTermF3 & 1) ? 0xA : ~0xA;
  funcBinRepAB &= (miniTermF3 & 2) ? 0xC : ~0xC;
  //init with 0xC because D is taken without inversion
  // D connected to input 0, C connected to input 1
  uint8_t funcBinRepDC = 0xA;
  funcBinRepDC &= (miniTermF3 & 4) ? 0xC : ~0xC;

  auto canonTtAB = getCanonF<2>(funcBinRepAB);
  auto canonTtDC = getCanonF<2>(funcBinRepDC);
  //It is assumed that P2 space is complete
  assert(existingCttP2.count(canonTtAB.ctt));
  assert(existingCttP2.count(canonTtDC.ctt));

  //TODO: don't allways take first cell with matching ctt, compare them
  auto exCellAB = existingCttP2[canonTtAB.ctt][0];
  auto exCellDC = existingCttP2[canonTtDC.ctt][0];

  const auto &cellAB = *exCellAB.first;
  auto outputAB = exCellAB.second;
  const auto &cellDC = *exCellDC.first;
  auto outputDC = exCellDC.second;

  //Connect cells in subnet
  model::SubnetBuilder builder;
  model::SubnetBuilder::LinkList linksAB;
  model::Subnet::LinkList linksDC;

  linksAB.push_back(builder.addInput()); //inputA
  linksAB.push_back(builder.addInput()); //inputB
  const auto inputC = builder.addInput();
  model::SubnetBuilder::LinkList permLinksAB(2);
  for (size_t i = 0; i < cellAB.transform[outputAB].permutation.size(); ++i) {
    auto permId = cellAB.transform[outputAB].permutation[i];
    permLinksAB[i] = linksAB[permId];
  }

  size_t fanoutAB = cellAB.outputPins.size();
  if (fanoutAB > 1) {
    const auto outputs =
      builder.addMultiOutputCell(cellAB.cellTypeID, permLinksAB);
    linksDC.push_back(outputs[outputAB]);
  } else {
    linksDC.push_back(builder.addCell(cellAB.cellTypeID, permLinksAB));
  }

  linksDC.push_back(inputC);
  size_t inputCofDC = 0;
  model::SubnetBuilder::LinkList permLinksDC(2);
  for (size_t i = 0; i < cellDC.transform[outputDC].permutation.size(); ++i) {
    auto permId = cellDC.transform[outputDC].permutation[i];
    permLinksDC[i] = linksDC[permId];
    if (permId == 1) {
      inputCofDC = i;
    }
  }

  size_t fanoutDC = cellDC.outputPins.size();
  if (fanoutDC > 1) {
    const auto outputs =
      builder.addMultiOutputCell(cellDC.cellTypeID, permLinksDC);
    builder.addOutput(outputs[outputDC]);
  } else {
    builder.addOutput(builder.addCell(cellDC.cellTypeID, permLinksDC));
  }

  const auto subnetID = builder.make();

  //std::cout << model::Subnet::get(subnetID) << std::endl;


  //register subnet
  const auto &cellTypeAB = model::CellType::get(cellAB.cellTypeID);
  const auto &cellTypeDC = model::CellType::get(cellDC.cellTypeID);
  //FIXME: this assumes pin assignments inside
  const auto attrID = createP2F3PropertiesAttr(cellAB, cellDC);
  std::string p2CellName = cellTypeAB.getName() + "*&*" + cellTypeDC.getName();

  const auto p2CellTypeID = model::makeCellType(
    model::CellSymbol::UNDEF,
    p2CellName,
    subnetID,
    attrID,
    model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
    3, // TODO this might be wrong
    1);

  // create new comb cell
  std::vector<kitty::dynamic_truth_table> ctt;
  std::vector<util::NpnTransformation> t;

  const auto &p2CellType = model::CellType::get(p2CellTypeID);
  // Calculate new truth table for the cell.
  const auto tt = model::evaluate(model::Subnet::get(subnetID));
  for (uint i = 0; i < p2CellType.getOutNum(); i++) {
    auto config = kitty::exact_p_canonization(tt[i]);
    ctt.push_back(util::getTT(config)); // canonized TT
    t.push_back(util::getTransformation(config));
  }
  //TODO: newly created supercell should have all fields properly filled
  StandardCell newSuperCell;
  newSuperCell.cellTypeID = p2CellTypeID;
  newSuperCell.ctt = ctt;
  newSuperCell.transform = t;
  newSuperCell.name = p2CellName;
  //TODO FIXME: properly fill other newSuperCell fields
  newSuperCell.propertyArea = cellAB.propertyArea + cellDC.propertyArea;
  newSuperCell.propertyDelay = cellAB.propertyDelay + cellDC.propertyDelay;
  newSuperCell.propertyLeakagePower = cellAB.propertyLeakagePower +
                                      cellDC.propertyLeakagePower;
  newSuperCell.inputPins = cellAB.inputPins;
  auto pin = cellDC.inputPins[inputCofDC];
  pin.name = cellDC.name + "." + pin.name;
  newSuperCell.inputPins.push_back(pin);
  newSuperCell.outputPins.push_back(cellDC.outputPins[outputDC]);

  pComplCells_.push_back(newSuperCell);
  //FIXME: correct ctt is not allways at 0 pos
  return {&pComplCells_.back(), 0};
}

static model::CellTypeAttrID createF3TermPropertiesAttr(
    const std::vector<std::pair<const StandardCell*,uint16_t>> &cellEquivalents,
    const StandardCell &cellOr) {

  model::PhysicalProperties newProps;
  for (const auto &cellPair : cellEquivalents) {
    const auto &cell = *cellPair.first;
    newProps.area += cell.propertyArea;
    newProps.delay += cell.propertyDelay;
    newProps.power += cell.propertyLeakagePower;
  }
  //const auto ports = getPorts(cellSrc);
  model::CellType::PortVector ports;
  size_t index{0};

  //hardcode 3 inputs for 3 variable function
  ports.emplace_back("INPUT_A", 1, true, index++);
  ports.emplace_back("INPUT_B", 1, true, index++);
  ports.emplace_back("INPUT_C", 1, true, index++);
  //register output pin
  ports.emplace_back("OUTPUT_Y", 1, false, index++);


  //TODO: this might break
  const auto attrID = model::makeCellTypeAttr(ports);
  model::CellTypeAttr::get(attrID).setPhysProps(newProps);
  return attrID;
}

SCLibrary::CellLogPair SCLibrary::buildF3termEquivalentCell(
    const std::vector<CanonInfo> &termCanons,
    const std::vector<CellLogPair> &cellEquivalents,
    CttMap &existingCttP2) {
  //find OR in P2 space
  uint8_t funcBinRepOr = 0xE;
  auto canonTtOr = getCanonF<2>(funcBinRepOr);
  //It is assumed that P2 space is complete
  assert(existingCttP2.count(canonTtOr.ctt));
  //TODO: don't allways take first cell with matching ctt, compare them
  auto exCellOr = existingCttP2[canonTtOr.ctt][0];
  const auto &cellOr = *exCellOr.first;
  auto cellOrOutput = exCellOr.second;
  // make connections in form of binary tree to reduce delay
  model::SubnetBuilder builder;
  model::SubnetBuilder::LinkList inputLinks;
  inputLinks.push_back(builder.addInput());
  inputLinks.push_back(builder.addInput());
  inputLinks.push_back(builder.addInput());

  // connect inputs to miniterm cellEquivalents
  model::SubnetBuilder::LinkList internalOutLinks;
  for (size_t i = 0; i < cellEquivalents.size(); ++i) {
    const auto& cell = *cellEquivalents[i].first;
    const auto cellOutput = cellEquivalents[i].second;
    const auto& term = termCanons[i];

    // term --<perm>-- canon function --<perm>-- StandardCell
    model::SubnetBuilder::LinkList termPermLinks(3);
    size_t termPermSize = term.transform.permutation.size();
    for (size_t i = 0; i < termPermSize; ++i) {
      auto permId = term.transform.permutation[i];
      termPermLinks[i] = inputLinks[permId];
    }
    //TODO: why this reverse is needed?
    std::reverse(termPermLinks.begin(), termPermLinks.end());

    model::SubnetBuilder::LinkList cellPermLinks(3);
    size_t cellPermSize = cell.transform[cellOutput].permutation.size();
    for (size_t i = 0; i < cellPermSize; ++i) {
      auto permId = cell.transform[cellOutput].permutation[i];
      cellPermLinks[i] = termPermLinks[permId];
    }
    //TODO: why this reverse is needed?
    std::reverse(cellPermLinks.begin(), cellPermLinks.end());

    size_t fanout = cell.outputPins.size();
    model::SubnetBuilder::Link outputLink;
    if (fanout > 1) {
      const auto outputs =
        builder.addMultiOutputCell(cell.cellTypeID, cellPermLinks);
      outputLink = outputs[cellOutput];
    } else {
      outputLink = builder.addCell(cell.cellTypeID, cellPermLinks);
    }
    internalOutLinks.push_back(outputLink);
  }

  // connect pair of terms to OR element. If number of terms is odd,
  // last term will be connected to the las OR element before subnet output
  while (internalOutLinks.size() != 1) {
    internalOutLinks = connectLayerToORs(internalOutLinks, builder, exCellOr);
  }
  // finish creating subnet
  builder.addOutput(internalOutLinks[0]);
  const auto subnetID = builder.make();

  //std::cout << model::Subnet::get(subnetID) << std::endl;

  //register subnet
  const auto attrID = createF3TermPropertiesAttr(
    cellEquivalents, cellOr);
  std::string p3Name = cellEquivalents[0].first->name;
  for (size_t i = 1; i < cellEquivalents.size(); ++i) {
    const auto& cellPair  = cellEquivalents[i];
    p3Name += "*|*" + cellPair.first->name;
  }
  const auto p3CellTypeID = model::makeCellType(
    model::CellSymbol::UNDEF,
    p3Name,
    subnetID,
    attrID,
    model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
    3, // TODO this might be wrong
    1);

  // create new comb cell
  std::vector<kitty::dynamic_truth_table> ctt;
  std::vector<util::NpnTransformation> t;

  const auto &p3CellType = model::CellType::get(p3CellTypeID);
  // Calculate new truth table for the cell.
  const auto tt = model::evaluate(model::Subnet::get(subnetID));
  for (uint i = 0; i < p3CellType.getOutNum(); i++) {
    auto config = kitty::exact_p_canonization(tt[i]);
    ctt.push_back(util::getTT(config)); // canonized TT
    t.push_back(util::getTransformation(config));
  }

  StandardCell newSuperCell;
  newSuperCell.cellTypeID = p3CellTypeID;
  newSuperCell.ctt = ctt;
  newSuperCell.transform = t;
  newSuperCell.name = p3Name;
  //TODO FIXME: properly fill other newSuperCell fields

  newSuperCell.propertyArea = 0;
  newSuperCell.propertyDelay = 0;
  newSuperCell.propertyLeakagePower = 0;
  //FIXME: alot of input ports here, but only 3 in registered subnet ports
  for (const auto &cellPair : cellEquivalents) {
    const auto &cell = *cellPair.first;
    newSuperCell.propertyArea += cell.propertyArea;
    newSuperCell.propertyDelay += cell.propertyDelay;
    newSuperCell.propertyLeakagePower += cell.propertyLeakagePower;
  }
  for (auto pin : cellEquivalents[0].first->inputPins) {
    newSuperCell.inputPins.push_back(pin);
  }
  newSuperCell.outputPins.push_back(cellOr.outputPins[cellOrOutput]);
  pComplCells_.push_back(newSuperCell);

  //FIXME: correct ctt is not allways at 0 pos
  return {&pComplCells_.back(), 0};
}

void SCLibrary::completeP3classes(CttMap &existingCttP3,
                                  CttMap &existingCttP2) {
  // Find missing 3p classes
  auto p3Classes = generateP3classes();
  for (const auto &funcClass : existingCttP3) {
    p3Classes.erase(funcClass.first); //erasure is expensive!!!
  }
  // p3Classes now contain all of the missing classes
  for (const auto& func : p3Classes) {
    uint64_t funcBinRep = func._bits[0]; //kinda bad
    // process const 0 and const 1 cases
    if (funcBinRep == 0) {
      continue;
      assert(false && "TODO return const zero");
      //TODO return const zero
    }
    if (funcBinRep == 0xFF) {
      continue;
      assert(false && "TODO return const one");
      //TODO return const one
    }

    // build Sum-of-products canonical form
    std::vector<uint8_t> miniTerms = getMiniTerms(funcBinRep, 8);
    // get canon function class for each miniterm
    std::vector<CanonInfo> termCanonFunc = getF3CanonRep(miniTerms);
    // create subnet representing function from SoPcanonical form
    std::vector<CellLogPair > cellEquivalents;
    for (size_t i = 0, size = miniTerms.size(); i < size; ++i) {
      // some miniterms already exist in p3, get corresponding cells
      if (existingCttP3.count(termCanonFunc[i].ctt)) {
        //TODO: don't allways take first cell with matching ctt, compare them
        cellEquivalents.push_back(existingCttP3[termCanonFunc[i].ctt][0]);
      } else {
        // build p2 representation for each remaining term
        // p2 space should be complete so we are guaranteed to build one
        auto newCellTerm = buildP2CellForF3MiniTerm(miniTerms[i], existingCttP2);
        const auto &newctCtt = newCellTerm.first->ctt[0];
        assert(newctCtt == termCanonFunc[i].ctt);
        cellEquivalents.push_back(newCellTerm);
        // remember new P3 Ctt function
        existingCttP3[termCanonFunc[i].ctt].push_back(newCellTerm);
      }
    }

    // conect terms with OR to create function
    auto newCell = buildF3termEquivalentCell(termCanonFunc, cellEquivalents, existingCttP2);
    const auto &newCtt = newCell.first->ctt[0];
    assert(newCtt == func);
    existingCttP3[func].push_back(newCell);
  }
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

  size_t i = 0;
  for (auto tt : cell.ctt) {
    const auto strFunc = kitty::to_hex(tt);
    if (nInputs == 0) {
      if (strFunc == "0") {
        constZeroCells_.push_back({cell, i});
      } else if (strFunc == "1") {
        constOneCells_.push_back({cell, i});
      }
    } else {
      if (nInputs == 1 && strFunc == "1") {
        negCombCells_.push_back({cell, i});
      }
    }
    ++i;
  }
  combCells_.push_back(std::move(cell));
}

 std::pair<const StandardCell*, size_t> SCLibrary::findCheapestCell (
    const std::vector< std::pair<StandardCell, size_t>> &scs) const {
  float lowArea = MAXFLOAT;
  float lowPower = MAXFLOAT;
  std::pair<const StandardCell*, size_t> result = {nullptr, -1};
  for (const auto &cell : scs) {
    const auto &cellType = model::CellType::get(cell.first.cellTypeID);
    const auto &props = cellType.getAttr().getPhysProps();
    if ((props.area < lowArea) ||
        (props.area == lowArea && props.power < lowPower)) {
      lowArea = props.area;
      lowPower = props.power;
      result = {&cell.first, cell.second};
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

// Construction of SubnetBuilder for two cells: the cell "cellToAdd" is
// connected to the first input of the other cell.
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
    addSuperCell(cell, *properties_.cheapNegCell.first, superCells, 0);

    uint output = 0;
    for (const auto& ctt : properties_.cheapOneCell.first->ctt) {
      if (kitty::to_hex(ctt) == "1") {
        break;
      }
      output++;
    }
    assert(output < model::CellType::get(
                      properties_.cheapOneCell.first->cellTypeID).getOutNum());
    addSuperCell(cell, *properties_.cheapOneCell.first,superCells, output);

    output = 0;
    for (const auto& ctt : properties_.cheapZeroCell.first->ctt) {
      if (kitty::to_hex(ctt) == "0") {
        break;
      }
      output++;
    }
    assert(output < model::CellType::get(
                      properties_.cheapZeroCell.first->cellTypeID).getOutNum());
    addSuperCell(cell, *properties_.cheapZeroCell.first, superCells, output);
  }
  combCells_.insert(combCells_.end(), superCells.begin(), superCells.end());
}

void SCLibrary::addConstCells() {
  // Two LinkLists: for cellToAdd and for the cell.
  std::string baseNames[2] = {"ONE", "ZERO"};
  const std::pair<const StandardCell*, size_t> cheapCells[2] = {properties_.cheapOneCell,
                                                                properties_.cheapZeroCell};
  for (int i = 0; i < 2; ++i) {
    std::string &baseName = baseNames[i];
    const std::pair<const StandardCell*, size_t> &cheapCell = cheapCells[i];
    for (size_t inputLinkNum = 1; inputLinkNum < 2; ++ inputLinkNum) {
      model::SubnetBuilder builder;

      for(size_t i = 0; i < inputLinkNum; ++i) {
        builder.addInput();
      }
      size_t nOutputs = cheapCell.first->outputPins.size();
      if (nOutputs > 1) {
        auto outputs = builder.addMultiOutputCell(cheapCell.first->cellTypeID, {});
        builder.addOutput(outputs[cheapCell.second]);
      } else {
        auto output = builder.addCell(cheapCell.first->cellTypeID, {});
        builder.addOutput(output);
      }
      const auto subnetID = builder.make();

      model::PhysicalProperties newProps;
      newProps.area = cheapCell.first->propertyArea;
      newProps.delay = cheapCell.first->propertyDelay;
      newProps.power = cheapCell.first->propertyLeakagePower;

      model::CellType::PortVector ports;
      size_t index{0};

      for (size_t i = 0; i < inputLinkNum; ++i) {
        ports.emplace_back("PSEUDO" + std::to_string(i), 1, true, index++);
      }
      ports.emplace_back("OUTPUT_Y", 1, false, index++);
      const auto attrID = model::makeCellTypeAttr(ports);
      model::CellTypeAttr::get(attrID).setPhysProps(newProps);

      std::string name = "PSEUDO_" + baseName +
                          "_" + std::to_string(inputLinkNum) + "IN";
      // Obtain new CellTypeID for the supercell.
      const auto constCellTypeID = model::makeCellType(
        model::CellSymbol::UNDEF,
        name,
        subnetID,
        attrID,
        model::CellProperties{1, 0, 1, 0, 0, 0, 0, 0, 0},
        inputLinkNum, // TODO it may be reduced
        1);

      std::vector<kitty::dynamic_truth_table> ctt;
      std::vector<util::NpnTransformation> t;
      // Calculate new truth table for the supercell.
      const auto &constCellType = model::CellType::get(constCellTypeID);
      const auto tt = model::evaluate(model::Subnet::get(subnetID));
      for (uint i = 0; i < constCellType.getOutNum(); i++) {
        auto config = kitty::exact_p_canonization(tt[i]);
        ctt.push_back(util::getTT(config)); // canonized TT
        t.push_back(util::getTransformation(config));
      }

      //TODO: newly created supercell should have all fields properly filled
      StandardCell newConstCell = *cheapCell.first;
      newConstCell.cellTypeID = constCellTypeID;
      newConstCell.ctt = ctt;
      newConstCell.transform = t;
      newConstCell.name = name;
      //TODO FIXME: properly fill other newSuperCell fields
      combCells_.push_back(newConstCell);
    }
  }
}

} // namespace eda::gate::library
