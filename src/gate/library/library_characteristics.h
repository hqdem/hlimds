//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/model/cellattr.h"
#include "gate/model/celltype.h"

#include <kitty/kitty.hpp>

#include <readcells/ast.h>
#include <readcells/ast_parser.h>
#include <readcells/groups.h>

#include <string>
#include <vector>

namespace eda::gate::library {

class LibraryCharacteristics {
public:
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

  static std::vector<std::string> getCells();

  static model::CellType::PortVector getPorts(const std::string &name);

  static std::vector<std::string> getInputs(const std::string &name);
  static std::vector<std::string> getOutputs(const std::string &name);
  static kitty::dynamic_truth_table getFunction(const std::string &name);

  static bool isCombCell(const std::string &name);
  static bool isIsolateCell(const std::string &name);

  static float getArea(const std::string &name);
  static float getLeakagePower(const std::string &name);
  static std::vector<Delay> getDelay (
      const std::string &name,
      const std::vector<float> &inputTransTime,
      const float outputCap);
  static Delay getDelay(
      const std::string &name,
      const std::string &relPin,
      const float inputTransTime,
      const float outputCap);

  // FIXME:
  static model::PhysicalProperties getPhysProps(const std::string &name) {
    model::PhysicalProperties props;
    props.area = getArea(name);
    props.delay = 1.;
    props.power = getLeakagePower(name);
    return props;
  }

private:
  static std::string binOpToString(const Expr *lhs,
                                   const std::string &op,
                                   const Expr *rhs);
  static std::string exprToString(const Expr *expr);
  static bool areIdsInExpr(const std::string &expr,
                           const std::vector<std::string> &ids);
  static std::vector<std::string> getFunctions(const std::string &name);
  static float getLutValue(const LookupTable *lut,
                           const std::vector<std::size_t> &paramsID);
  static float getLutInterValue(const LookupTable *lut,
                                const std::vector<InterParamIDs> &paramsID,
                                const std::vector<float> &searchParams);
  static float getTwoAxisLutInterValue(
      const LookupTable *lut,
      const std::vector<InterParamIDs> &paramsID,
      const std::vector<float> &searchParams);
  static float getValue(const LookupTable *lut,
                        const std::vector<float> &searchParams);
};

} // namespace eda::gate::library
