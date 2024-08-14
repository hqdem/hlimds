//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/celltype.h"

#include <kitty/kitty.hpp>

#include <readcells/groups.h>

#include <string>
#include <vector>

namespace eda::gate::library {

class ReadCellsIface {
public:
  ReadCellsIface(Library &library) :
    library(library) {};

  struct Delay {
    float cellRise;
    float cellFall;
    float riseTransition;
    float fallTransition;
  };

  struct InterParamIDs {
    size_t lowerID = SIZE_MAX;
    size_t upperID = SIZE_MAX;
  };

  std::vector<std::string> getCells();

  model::CellType::PortVector getPorts(const std::string &name);

  std::vector<std::string> getInputs(const std::string &name);
  std::vector<std::string> getOutputs(const std::string &name);
  kitty::dynamic_truth_table getFunction(const std::string &name);

  bool isCombCell(const std::string &name);
  bool isIsolateCell(const std::string &name);

  float getArea(const std::string &name);
  float getLeakagePower(const std::string &name);
  std::vector<Delay> getDelay (
      const std::string &name,
      const std::vector<float> &inputTransTime,
      const float outputCap);
  Delay getDelay(
      const std::string &name,
      const std::string &relPin,
      const float inputTransTime,
      const float outputCap);

  // FIXME:
  model::PhysicalProperties getPhysProps(const std::string &name) {
    model::PhysicalProperties props;
    props.area = getArea(name);
    props.delay = 1.;
    props.power = getLeakagePower(name);
    return props;
  }

private:
  std::string binOpToString(const Expr *lhs,
                                   const std::string &op,
                                   const Expr *rhs);
  std::string exprToString(const Expr *expr);
  bool areIdsInExpr(const std::string &expr,
                           const std::vector<std::string> &ids);
  std::vector<std::string> getFunctions(const std::string &name);
  float getLutValue(const LookupTable *lut,
                           const std::vector<std::size_t> &paramsID);
  float getLutInterValue(const LookupTable *lut,
                                const std::vector<InterParamIDs> &paramsID,
                                const std::vector<float> &searchParams);
  float getTwoAxisLutInterValue(
      const LookupTable *lut,
      const std::vector<InterParamIDs> &paramsID,
      const std::vector<float> &searchParams);
  float getValue(const LookupTable *lut,
                        const std::vector<float> &searchParams);
  Library &library;
};

} // namespace eda::gate::library
