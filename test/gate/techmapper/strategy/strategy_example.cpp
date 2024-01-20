#include "gate/techmapper/strategy/strategy_example.h"
#include "gate/model2/celltype.h"
#include "gate/model2/net.h"
#include "gate/optimizer2/resynthesis/isop.h"

#include <vector>

using SubnetBuilder = eda::gate::model::SubnetBuilder;
using Subnet = eda::gate::model::Subnet;

namespace eda::gate::tech_optimizer {
  using CellTypeID = eda::gate::model::CellTypeID;

  CellDB getSimpleCells(){
    using NetID = eda::gate::model::NetID;
    using MinatoMorrealeAlg = eda::gate::optimizer2::resynthesis::MinatoMorrealeAlg;
    
    std::vector<CellTypeID> cellTypeIDs;

    eda::gate::model::CellProperties props(true, false, false, false, false, false, false);
    eda::gate::model::CellTypeAttrID attrID;
    MinatoMorrealeAlg minatoMorrealeAlg;

    // Add AND 
    kitty::dynamic_truth_table truthTableAND(2);
    kitty::create_from_binary_string(truthTableAND, "10000");

    const auto subnetIDAND = minatoMorrealeAlg.synthesize(truthTableAND);

    NetID netIDAND = static_cast<NetID>(subnetIDAND);

    CellTypeID cellIDAND = eda::gate::model::makeCellType(
        "CustomAND", netIDAND, attrID,
        eda::gate::model::CellSymbol::CELL,
        props, static_cast<uint16_t>(2), 
        static_cast<uint16_t>(1));

    cellTypeIDs.push_back(cellIDAND);

    // Add NOT
    kitty::dynamic_truth_table truthTable(1);
    kitty::create_from_binary_string(truthTable, "01");

    const auto subnetID = minatoMorrealeAlg.synthesize(truthTable);

    NetID netID = static_cast<NetID>(subnetID);

    CellTypeID cellID = eda::gate::model::makeCellType(
        "CustomNOT", netID, attrID,
        eda::gate::model::CellSymbol::CELL,
        props, static_cast<uint16_t>(1), 
        static_cast<uint16_t>(1));

    cellTypeIDs.push_back(cellID);

    return CellDB(cellTypeIDs);
  }

  SubnetID subnet1() {
    using Link = Subnet::Link;
    using LinkList = Subnet::LinkList;

    size_t arity = 5;
 
    SubnetBuilder builder;
    LinkList links;

    for (size_t i = 0; i < arity; i++) {
      const auto idx = builder.addInput();
      links.emplace_back(Link(idx));
    }

    size_t idx = 0;
    idx = builder.addCell(eda::gate::model::CellSymbol::AND, links);

    builder.addOutput(idx);

    return builder.make();
  }

  //SubnetID subnet2();

} // namespace eda::gate::optimizer
