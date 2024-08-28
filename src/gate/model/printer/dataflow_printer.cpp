//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/printer/dataflow_printer.h"

namespace eda::gate::model {

/**
 * @brief Returns true if the i-th subnet links PI and PO from the builder,
 * false otherwise.
 */
static bool isInOutLink(const DesignBuilder &builder, const size_t i) {
  const auto inOutInner = builder.getCellNum(i, true);
  uint32_t inN = std::get<0>(inOutInner);
  uint32_t outN = std::get<1>(inOutInner);
  uint32_t innerN = std::get<2>(inOutInner);
  return builder.subnetLinkedWithPI(i) && builder.subnetLinkedWithPO(i) &&
      !innerN && inN == 1 && outN == 1;
}

/**
 * @brief Prints vertices labels, finds subnet links, minimum and maximum
 * subnet size.
 */
static void printLabels(
    std::ostream &out,
    const DesignBuilder &builder,
    std::vector<std::vector<size_t>> &subnetLinks,
    uint32_t &maxSubnetSize) {

  const std::string labelHead = " [label=\"";
  const std::string labelTail = "\"];\n";
  const size_t nSubnet = builder.getSubnetNum();
  const size_t designInNum = builder.getInNum(),
               designOutNum = builder.getOutNum();

  out << 0 << labelHead << "0 (" << designInNum << ", 0)" << labelTail;
  for (size_t i = 0; i < nSubnet; ++i) {
    const auto inOutInner = builder.getCellNum(i, true);
    uint32_t inN = std::get<0>(inOutInner);
    uint32_t outN = std::get<1>(inOutInner);
    uint32_t innerN = std::get<2>(inOutInner);
    if (isInOutLink(builder, i)) {
      continue;
    }
    if (!i) {
      maxSubnetSize = innerN;
    } else {
      maxSubnetSize = std::max(innerN, maxSubnetSize);
    }

    out << i + 1 << labelHead << innerN << " (" << inN << ", " << outN << ")" <<
        labelTail;
    const auto &outArcs = builder.getOutArcs(i);
    for (const auto outArc : outArcs) {
      subnetLinks[outArc].push_back(i);
    }
  }
  out << nSubnet + 1 << labelHead << "0 (0, " << designOutNum << ")" <<
      labelTail;
}

/// @brief Finds the vertices hue according to the number of cells inside.
static float findHue(
    const uint32_t cellsNum,
    const uint32_t maxSubnetSize) {
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

/// @brief Prints vertices colors.
static void printColors(
    std::ostream &out,
    const DesignBuilder &builder,
    const uint32_t maxSubnetSize) {

  const std::string colorHead = " [style=filled, color=\"";
  const std::string colorTail = " 0.8 1.0\"];\n";
  const size_t nSubnet = builder.getSubnetNum();
  float insHue = findHue(0, maxSubnetSize);
  out << 0 << colorHead << insHue << colorTail;
  for (size_t i = 0; i < nSubnet; ++i) {
    const auto inOutInner = builder.getCellNum(i, true);
    uint32_t innerN = std::get<2>(inOutInner);
    if (isInOutLink(builder, i)) {
      continue;
    }

    float hue = findHue(innerN, maxSubnetSize);
    out << i + 1 << colorHead << hue << colorTail;
  }
  float outsHue = findHue(0, maxSubnetSize);
  out << nSubnet + 1 << colorHead << outsHue << colorTail;
}

/// @brief Prints directed arcs between vertices.
static void printArcs(
    std::ostream &out,
    const DesignBuilder &builder,
    const std::vector<std::vector<size_t>> &subnetLinks) {

  const size_t nSubnet = builder.getSubnetNum();
  bool inOutLink = false;
  for (size_t i = 0; i < nSubnet; ++i) {
    if (isInOutLink(builder, i)) {
      inOutLink = true;
      continue;
    }
    if (builder.subnetLinkedWithPI(i)) {
      out << 0 << " -> " << i + 1 << ";\n";
    }
    if (builder.subnetLinkedWithPO(i)) {
      out << i + 1 << " -> " << nSubnet + 1 << ";\n";
    }
    for (const auto link : subnetLinks[i]) {
      out << link + 1 << " -> " << i + 1 << ";\n";
    }
  }
  if (inOutLink) {
    out << 0 << " -> " << nSubnet + 1 << ";\n";
  }
}

std::ostream &operator <<(std::ostream &out, const DesignBuilder &builder) {
  out << "digraph " << builder.getName() << " {\n";
  out << "graph [ranksep=2.0];\n";

  const size_t nSubnet = builder.getSubnetNum();
  uint32_t maxSubnetSize = 0;
  std::vector<std::vector<size_t>> subnetLinks(nSubnet);

  printLabels(out, builder, subnetLinks, maxSubnetSize);
  printColors(out, builder, maxSubnetSize);
  printArcs(out, builder, subnetLinks);

  out << "}\n";
  return out;
}

} // namespace eda::gate::model
