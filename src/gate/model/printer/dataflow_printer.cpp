//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/printer/dataflow_printer.h"

namespace eda::gate::model {

const std::string labelHead = " [label=\"";
const std::string labelTail = "\"];\n";

const std::string colorHead = " [style=filled, color=\"";
const std::string colorTail = " 0.8 1.0\"];\n";

/**
 * @brief Returns true if the i-th subnet links PI and PO from the builder and
 * has no inner cells, false otherwise.
 */
static bool isInOutLink(const DesignBuilder &builder, const size_t i) {
  const auto inOutInner = builder.getCellNum(i, true);
  const auto &entry = builder.getEntry(i);
  uint32_t inN = std::get<0>(inOutInner);
  uint32_t outN = std::get<1>(inOutInner);
  uint32_t innerN = std::get<2>(inOutInner);
  return entry.hasPIArc() && entry.hasPOArc() &&
      !innerN && inN == 1 && outN == 1;
}

/// Prints primary inputs label.
static void printPILabel(std::ostream &out, const DesignBuilder &builder) {
  const size_t designInNum = std::get<0>(builder.getCellNum(true));
  out << DesignBuilder::PISubnetEntryIdx << labelHead << 0 << " (" <<
      designInNum << ", " << 0 << ")" << labelTail;
}

/// Prints primary outputs label.
static void printPOLabel(std::ostream &out, const DesignBuilder &builder) {
  const size_t designOutNum = std::get<1>(builder.getCellNum(true));
  out << DesignBuilder::POSubnetEntryIdx << labelHead << 0 << " (" <<
      0 << ", " << designOutNum << ")" << labelTail;
}

/// Prints subnet entry label.
static void printEntryLabel(
    std::ostream &out,
    const DesignBuilder &builder,
    const size_t entryIdx) {

  const auto inOutInner = builder.getCellNum(entryIdx, true);
  uint32_t inN = std::get<0>(inOutInner);
  uint32_t outN = std::get<1>(inOutInner);
  uint32_t innerN = std::get<2>(inOutInner);

  out << entryIdx << labelHead << innerN << " (" << inN << ", " << outN <<
      ")" << labelTail;
}

/// Finds hue according to the number of cells inside the node.
static float findHue(const uint32_t cellsNum, const uint32_t maxSubnetSize) {
  float hue = 0.f;
  if (maxSubnetSize >= 1e-6) {
    float ratio = static_cast<float>(cellsNum) / maxSubnetSize;
    hue = 0.3f - 0.3f * ratio;
  }
  if (0.f - hue > 1e-6) {
    return 0.f;
  }
  return hue;
}

/// Prints nodes' colors.
static void printColors(
    std::ostream &out,
    const DesignBuilder &builder,
    std::unordered_set<size_t> &printedLabels,
    const uint32_t maxSubnetSize) {

  const size_t nSubnet = builder.getSubnetNum();
  float insHue = findHue(0, maxSubnetSize);
  out << DesignBuilder::PISubnetEntryIdx << colorHead << insHue << colorTail;
  for (size_t i = 0; i < nSubnet; ++i) {
    const auto inOutInnerN = builder.getCellNum(i, true);
    uint32_t innerN = std::get<2>(inOutInnerN);
    if (isInOutLink(builder, i) ||
        printedLabels.find(i) == printedLabels.end()) {
      continue;
    }
    float hue = findHue(innerN, maxSubnetSize);
    out << i << colorHead << hue << colorTail;
  }
  float outsHue = findHue(0, maxSubnetSize);
  out << DesignBuilder::POSubnetEntryIdx << colorHead << outsHue << colorTail;
}

static void printArc(
    std::ostream &out,
    const DesignBuilder &builder,
    std::unordered_set<size_t> &printedLabel,
    const size_t from,
    const size_t to) {

  if (printedLabel.find(from) == printedLabel.end()) {
    if (from == DesignBuilder::PISubnetEntryIdx) {
      printPILabel(out, builder);
    } else {
      printEntryLabel(out, builder, from);
    }
    printedLabel.insert(from);
  }
  if (printedLabel.find(to) == printedLabel.end()) {
    if (to == DesignBuilder::POSubnetEntryIdx) {
      printPOLabel(out, builder);
    } else {
      printEntryLabel(out, builder, to);
    }
    printedLabel.insert(to);
  }
  out << from << " -> " << to << ";\n";
}

/// @brief Prints directed arcs between nodes and finds max node size.
static void printAllArcs(
    std::ostream &out,
    const DesignBuilder &builder,
    std::unordered_set<size_t> &printedLabels,
    SubnetSz &maxSubnetSize) {

  const size_t nSubnet = builder.getSubnetNum();
  bool inOutLink = false;
  for (size_t i = 0; i < nSubnet; ++i) {
    const auto entry = builder.getEntry(i);
    maxSubnetSize =
        std::max(maxSubnetSize, std::get<2>(builder.getCellNum(i, true)));

    if (isInOutLink(builder, i)) {
      inOutLink = true;
      continue;
    }
    if (entry.hasPIArc() &&
        entry.getPIArcDesc().signalType == SignalType::DATA) {
      printArc(out, builder, printedLabels, DesignBuilder::PISubnetEntryIdx, i);
    }
    if (entry.hasPOArc() &&
        entry.getPOArcDesc().signalType == SignalType::DATA) {
      printArc(out, builder, printedLabels, i, DesignBuilder::POSubnetEntryIdx);
    }
    for (const auto link : entry.getInArcs()) {
      if (entry.getArcDesc(link).signalType == SignalType::DATA) {
        printArc(out, builder, printedLabels, link, i);
      }
    }
  }
  if (inOutLink) {
    printArc(out, builder, printedLabels,
        DesignBuilder::PISubnetEntryIdx,
        DesignBuilder::POSubnetEntryIdx);
  }
  if (printedLabels.find(DesignBuilder::PISubnetEntryIdx) ==
      printedLabels.end()) {
    printPILabel(out, builder);
  }
  if (printedLabels.find(DesignBuilder::POSubnetEntryIdx) ==
      printedLabels.end()) {
    printPOLabel(out, builder);
  }
}

std::ostream &operator <<(std::ostream &out, const DesignBuilder &builder) {
  out << "digraph " << builder.getName() << " {\n";
  out << "graph [ranksep=2.0];\n";

  SubnetSz maxSubnetSize = 0;
  std::unordered_set<size_t> printedLabels;

  printAllArcs(out, builder, printedLabels, maxSubnetSize);
  printColors(out, builder, printedLabels, maxSubnetSize);

  out << "}\n";
  return out;
}

} // namespace eda::gate::model
