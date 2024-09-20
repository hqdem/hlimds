//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/estimator/ppa_estimator.h"
#include "gate/estimator/probabilistic_estimate.h"
#include "shell/shell.h"

namespace eda::shell {

template <typename T>
static inline void printNameValue(const std::string &name,
                                  const T &value,
                                  const std::string &suffix = "") {
  UTOPIA_SHELL_OUT << std::setw(8) << std::left << name
                   << std::fixed << value
                   << suffix
                   << std::endl;
}

struct StatDesignCommand final : public UtopiaCommand {
  StatDesignCommand(): UtopiaCommand(
      "stat_design", "Prints the design characteristics") {
    app.allow_extras();
  }

  int run(Tcl_Interp *interp, int argc, const char *argv[]) override {
    namespace estimator = eda::gate::estimator;

    UTOPIA_SHELL_ERROR_IF_NO_DESIGN(interp);
    UTOPIA_SHELL_PARSE_ARGS(interp, app, argc, argv);

    const bool isTechMapped = getDesign()->isTechMapped();

    size_t nIn{0}, nOut{0}, nInt{0}, depth{0};
    float area{0}, delay{0}, power{0}, activ{0};

    std::tie(nIn, nOut, nInt) = getDesign()->getCellNum(false);
    const size_t nCell = nIn + nOut + nInt;

    for (size_t i = 0; i < getDesign()->getSubnetNum(); ++i) {
      const auto &subnetID = getDesign()->getSubnetID(i);
      const auto &subnet = eda::gate::model::Subnet::get(subnetID);

      const auto builder = std::make_shared<gate::model::SubnetBuilder>(subnet);
      eda::gate::estimator::ProbabilityEstimator estimator;
      eda::gate::estimator::SwitchActivity::Probabilities probs;
      eda::gate::estimator::SwitchActivity activity;

      estimator.estimate(builder, probs, activity);
      activ += activity.getSwitchProbsSum();
      depth = std::max<size_t>(subnet.getPathLength().second, depth);

      if (isTechMapped) {
        const auto &pLibrary = context->techMapContext.library;
        UTOPIA_SHELL_ERROR_IF(interp, !pLibrary,
                              "can't access techMap library");
        area += estimator::getArea(subnetID);
        power += estimator::getLeakagePower(subnetID, *pLibrary);
        delay = std::max<float>(estimator::getArrivalTime(subnetID, *pLibrary),
                                delay);
      }
    } // for subnet

    printNameValue("Design", fmt::format("'{}'", getDesign()->getName()));
    printNameValue("PIs", nIn);
    printNameValue("POs", nOut);
    printNameValue("Subnets", getDesign()->getSubnetNum());
    printNameValue("Cells", nCell, " (incl. PI/PO)");
    printNameValue("", nInt);
    printNameValue("Depth", depth);
    printNameValue("SwActiv", activ);

    if (isTechMapped) {
      printNameValue("Area", area, " um^2");
      printNameValue("Delay", delay, " ns");
      printNameValue("Power", power, " uW");
    }

    UTOPIA_SHELL_OUT << std::flush;
    return TCL_OK;
  }
};

} // namespace eda::shell
