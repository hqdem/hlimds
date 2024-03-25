//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/techoptimizer/hard_ip_mapper/hard_ip_mapper.h"

#include <cassert>

namespace eda::gate::tech_optimizer {

HardIPMapper::HardIPMapper(CellDB *cells) {
  this->cells = cells;
}

model::SubnetID HardIPMapper::mapHardIPCell(std::string name, Techmapper::MapperType techmapSelector) {
  parse(name);


  auto& sequenceCell = model::Cell::get(sequenceCellID);
  assert(sequenceCell.isDff() || sequenceCell.isDffRs() || sequenceCell.isLatch());

  if (sequenceCell.isDff()) {
    return mapDFF(techmapSelector);
  } else if (sequenceCell.isDffRs()) {
    return mapDFFrs(techmapSelector);
  } else if (sequenceCell.isLatch()) {
    return mapLatch(techmapSelector);
  }

  return model::SubnetID{}; // Assuming model::SubnetID{} is a valid default or error value
}

HardIPMapper::HardIPCell HardIPMapper::parse(std::string name) {
  HardIPCell cell;

  // Определение типа
  if (name.find("add") == 0) {
    cell.type = HardIPMType::ADD;
  } else if (name.find("mux") == 0) {
    cell.type = HardIPMType::MUX;
  }

  // Парсинг входов и выходов
  std::size_t iPos = name.find("_i");
  while (iPos != std::string::npos) {
    std::size_t nextPos = name.find("_", iPos + 1);
    int bits = std::stoi(name.substr(iPos + 2, nextPos - (iPos + 2)));
    cell.inputs.push_back(bits);
    iPos = name.find("_i", iPos + 1);
  }

  std::size_t oPos = name.find("_o");
  while (oPos != std::string::npos) {
    std::size_t nextPos = name.find("_", oPos + 1);
    if (nextPos == std::string::npos) {
      nextPos = name.length();
    }
    int bits = std::stoi(name.substr(oPos + 2, nextPos - (oPos + 2)));
    cell.outputs.push_back(bits);
    oPos = name.find("_o", oPos + 1);
  }

  return cell;
}

model::SubnetID HardIPMapper::crateADD(HardIPMapper::HardIPCell hardIpCell) {
  model::SubnetBuilder subnetBuilder;
  auto maxIt = std::max_element(hardIpCell.inputs.begin(), hardIpCell.inputs.end());

  std::vector<std::vector<model::Subnet::Link>> inputs;
  for (const auto &in : hardIpCell.inputs) {
    for (int i = 0; i < in; i++) {
      inputs[in][i] = subnetBuilder.addInput();
    }
  }

  std::vector<model::Subnet::Link> carryFlag;
  for (int i = 0; i < *maxIt; i++) {
    model::Subnet::LinkList linkList;
    for (const auto &in : hardIpCell.inputs) {
      linkList.push_back(inputs[in][i]);
    }
    if (i != 0) {
      for (const auto &link: carryFlag) {
        linkList.push_back(link);
      }
    }
    subnetBuilder.addCell(model::CellSymbol::XOR, linkList);

    for (const auto &link: linkList)
    carryFlag = subnetBuilder.addCell(model::CellSymbol::AND, linkList);
  }

}
model::SubnetID SequentialMapper::mapLatch(Techmapper::MapperType techmapSelector) {
  return chooseMappingStrategy(cells->getLatch(), techmapSelector);
}

model::SubnetID SequentialMapper::mapDFFrs(Techmapper::MapperType techmapSelector) {
  return chooseMappingStrategy(cells->getDFFrs(), techmapSelector);
}

model::SubnetID SequentialMapper::mapDFF(Techmapper::MapperType techmapSelector) {
  return chooseMappingStrategy(cells->getDFF(), techmapSelector);
}

model::SubnetID SequentialMapper::chooseMappingStrategy(const std::vector<std::pair<model::SubnetID, Subnetattr>>& seqCells,
                                                        Techmapper::MapperType techmapSelector) {
  switch (techmapSelector) {
    case Techmapper::MapperType::SIMPLE_AREA_FUNC:
      return areaOptimizedMapping(seqCells);
      /* More strategies can be added here */
    default:
      return model::SubnetID{}; // Assuming model::SubnetID{} is a valid default or error value
  }
}

model::SubnetID SequentialMapper::areaOptimizedMapping(const std::vector<std::pair<model::SubnetID, Subnetattr>>& seqCells) {
  std::pair<model::SubnetID, Subnetattr> minAreaCell =
      *std::min_element(seqCells.begin(), seqCells.end(),
                        [](const auto& lhs, const auto& rhs) {
                          return lhs.second.area < rhs.second.area;});
  return minAreaCell.first;
}

} // namespace eda::gate::tech_optimizer
