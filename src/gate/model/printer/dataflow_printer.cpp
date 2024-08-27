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
 * @brief Prints vertices labels, finds subnet links, minimum and maximum
 * subnet size.
 */
static void printLabels(
    std::ostream &out,
    const DesignBuilder &builder,
    std::vector<std::vector<size_t>> &subnetLinks,
    uint32_t &minSubnetSize, uint32_t &maxSubnetSize) {

  const std::string printHead = " [label=\"";
  const std::string printTail = "\"];\n";
  const size_t nSubnet = builder.getSubnetNum();
  const size_t designInNum = builder.getInNum(),
               designOutNum = builder.getOutNum();

  out << 0 << printHead << "PIs_" << designInNum << printTail;
  for (size_t i = 0; i < nSubnet; ++i) {
    const auto &entry = builder.getEntry(i);
    uint32_t subnetSize = 0;
    if (entry.subnetID != OBJ_NULL_ID) {
      subnetSize = Subnet::get(entry.subnetID).getCellNum();
    } else {
      subnetSize = entry.builder->getCellNum();
    }
    if (!i) {
      minSubnetSize = subnetSize;
      maxSubnetSize = subnetSize;
    } else {
      minSubnetSize = std::min(subnetSize, minSubnetSize);
      maxSubnetSize = std::max(subnetSize, maxSubnetSize);
    }

    out << i + 1 << printHead << "snet_" << subnetSize << printTail;
    const auto &outArcs = builder.getOutArcs(i);
    for (const auto outArc : outArcs) {
      subnetLinks[outArc].push_back(i);
    }
  }
  minSubnetSize = std::min((size_t)minSubnetSize,
      std::min(designInNum, designOutNum));
  maxSubnetSize = std::max((size_t)maxSubnetSize,
      std::min(designInNum, designOutNum));
  out << nSubnet + 1 << printHead << "POs_" << designOutNum << printTail;
}

/// @brief Finds the vertices hue according to the number of cells inside.
static float findHue(
    const uint32_t cellsNum,
    const uint32_t minSubnetSize, const uint32_t maxSubnetSize) {
  float hue = 0.f;
  if (std::fabs(maxSubnetSize - minSubnetSize) >= 1e-6) {
    float ratio = static_cast<float>(cellsNum - minSubnetSize) /
        (maxSubnetSize - minSubnetSize);
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
    const uint32_t minSubnetSize, const uint32_t maxSubnetSize) {

  const std::string printHead = " [style=filled, color=\"";
  const std::string printTail = " 0.8 1.0\"];\n";
  const size_t nSubnet = builder.getSubnetNum();
  float insHue = findHue(builder.getInNum(), minSubnetSize, maxSubnetSize);
  out << 0 << printHead << insHue << printTail;
  for (size_t i = 0; i < nSubnet; ++i) {
    const auto &entry = builder.getEntry(i);
    uint32_t subnetSize = 0;
    if (entry.subnetID != OBJ_NULL_ID) {
      subnetSize = Subnet::get(entry.subnetID).getCellNum();
    } else {
      subnetSize = entry.builder->getCellNum();
    }

    float hue = findHue(subnetSize, minSubnetSize, maxSubnetSize);
    out << i + 1 << printHead << hue << printTail;
  }
  float outsHue = findHue(builder.getOutNum(), minSubnetSize, maxSubnetSize);
  out << nSubnet + 1 << printHead << outsHue << printTail;
}

/// @brief Prints directed arcs between vertices.
static void printArcs(
    std::ostream &out,
    const DesignBuilder &builder,
    const std::vector<std::vector<size_t>> &subnetLinks) {

  const size_t nSubnet = builder.getSubnetNum();
  for (size_t i = 0; i < nSubnet; ++i) {
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
}

std::ostream &operator <<(std::ostream &out, const DesignBuilder &builder) {
  out << "digraph " << builder.getName() << " {\n";

  const size_t nSubnet = builder.getSubnetNum();
  uint32_t minSubnetSize = 0, maxSubnetSize = 0;
  std::vector<std::vector<size_t>> subnetLinks(nSubnet);

  printLabels(out, builder, subnetLinks, minSubnetSize, maxSubnetSize);
  printColors(out, builder, minSubnetSize, maxSubnetSize);
  printArcs(out, builder, subnetLinks);

  out << "}\n";
  return out;
}

} // namespace eda::gate::model
