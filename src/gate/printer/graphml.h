//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/model/gnet.h"

#include <ctemplate/template.h>

#include <map>
#include <ostream>
#include <string>

namespace eda::printer::graphml {

/**
* \brief Converts GNet to GraphMl representation.
* \author <a href="mailto:alex.sh2002@mail.ru">Alexsey Shtokman</a>
*/
class toGraphMl {
  using GNet = eda::gate::model::GNet;
  using Gate = eda::gate::model::Gate;
  using Link = Gate::Link;
  using GateSymbol = eda::gate::model::GateSymbol;
  public:
    static void printer(std::ostream &output, const GNet &model);
  private:
    static std::map<std::string, std::string> colours;

    static void setShape(const GateSymbol &gate,
        ctemplate::TemplateDictionary &dict);
    static std::string printNode(const Gate *node, const std::string colour);
    static std::string printEdge(const Link &link,
        const bool sourceHasNegation);
    static const std::string linkToString(const Link &link);
    static const std::string setGateSymbol(const GateSymbol &gate,
        const std::string colour);
};

} //namespace eda::printer::graphml
