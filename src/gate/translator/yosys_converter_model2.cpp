//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "gate/translator/yosys_converter_model2.h"

#include "fmt/ostream.h"

#include <algorithm>

namespace UModel = eda::gate::model;

namespace RTLIL = Yosys::RTLIL;

using Net = eda::gate::model::Net;

std::ostream YosysConverterModel2::devnull(nullptr);

YosysConverterModel2::YosysConverterModel2(const YosysToModel2Config &cfg)
    : debug(cfg.debugMode ? std::cerr : devnull) {
  Yosys::yosys_setup();
  RTLIL::Design design;
  Yosys::run_pass("design -reset-vlog", &design);
  std::string files;
  for (const auto &file: cfg.files) {
    files += file + " ";
  }
  files.pop_back();
  const std::string command = "read_verilog " + files;
  Yosys::run_pass(command.c_str(), &design);
  deterTopModule(design, cfg.topModule);
  Yosys::run_pass("proc", &design);
  Yosys::run_pass("opt -nodffe -nosdff", &design);
  Yosys::run_pass("memory", &design);
  Yosys::run_pass("opt -nodffe -nosdff", &design);
  Yosys::run_pass("pmuxtree", &design);
  Yosys::run_pass("splitnets -ports", &design);
  Yosys::run_pass("opt -mux_undef -mux_bool -undriven -fine", &design);
  readModules(design);
}

void YosysConverterModel2::deterTopModule(
    RTLIL::Design &design, const std::string &topModule) {
  if (topModule.empty()) {
    Yosys::run_pass("hierarchy -auto-top", &design);
    RTLIL::Module *module = design.top_module();
    nameTopModule = module->name.str();
    nameTopModule.erase(0, 1);
  } else {
    nameTopModule = topModule;
  }
}

UModel::NetID YosysConverterModel2::getNetID() {
  return listNetID.back();
}

YosysConverterModel2::~YosysConverterModel2() {
  for (auto &sig: signals) {
    delete sig;
  }
}

int YosysConverterModel2::getNewIndex() {
  curModule.indexNew--;
  return (curModule.indexNew + 1);
}

std::string YosysConverterModel2::operatorToString(Operator op) {
  switch (op) {
  case O_Add: return "add";
  case O_Sub: return "sub";
  case O_Mul: return "mul";
  case O_Div: return "div";
  case O_Not: return "not";
  case O_And: return "and";
  case O_Nand: return "nand";
  case O_Or: return "or";
  case O_Nor: return "nor";
  case O_Orr: return "orr";
  case O_Andr: return "andr";
  case O_Xorr: return "xorr";
  case O_Xor: return "xor";
  case O_Xnor: return "xnor";
  case O_Cat: return "cat";
  case O_Mux: return "mux";
  case O_Shr: return "shr";
  case O_Shl: return "shl";
  case O_Dshr: return "dshr";
  case O_Dshl: return "dshl";
  case O_Leq: return "leq";
  case O_Lt: return "lt";
  case O_Geq: return "geq";
  case O_Gt: return "gt";
  case O_Neg: return "neg";
  case O_Neq: return "neq";
  case O_Eq: return "eq";
  case O_Pad: return "pad";
  case O_Dff: return "dff";
  case O_Dffrs: return "dffrs";
  case O_Dffe: return "dffe";
  case O_Sdff: return "sdff";
  case O_Sdffe: return "sdffe";
  case O_Sdffce: return "sdffce";
  case O_Pmux: return "pmux";
  case O_Init: return "init";
  case O_Dlatch: return "dlatch";
  case O_Pos: return "pos";
  case O_Boolr: return "boolr";
  case O_Xnorr: return "xnorr";
  case O_Eqx: return "eqx";
  case O_Nex: return "nex";
  case O_Pow: return "pow";
  case O_Mod: return "REMs";
  case O_Divfloor: return "divfloor";
  case O_Modfloor: return "modfloor";
  case O_Adffe: return "adffe";
  case O_Aldffe: return "aldffe";
  case O_Dffsre: return "dffsre";
  case O_Aldff: return "aldff";
  case O_Dffsr: return "dffsr";
  case O_Adlatch: return "adlatch";
  case O_Dlatchsr: return "dlatchsr";
  case O_Sr: return "sr";
  default: return "Unknown Operator";
  }
}

void YosysConverterModel2::readModules(const RTLIL::Design &des) {
  const IdDict<RTLIL::Module *> &m_ = des.modules_;
  for (const auto &[str, module]: m_) {
    std::string nameModule = str.str().erase(0, 1);
    debug << fmt::format("Module:\n {} {} \n", nameModule, str.index_);
    modulesMap.emplace(str.index_, module);
    modulesNameMap.emplace(nameModule, module);
  }
  if (modulesNameMap.find(nameTopModule) != modulesNameMap.end()) {
    walkModule(modulesNameMap[nameTopModule]);
  } else {
    debug << nameTopModule << "\n";
    assert(0 && "The top module isn't exist");
  }
}

std::string YosysConverterModel2::readIdString(const RTLIL::IdString &str) {
  std::string id = "name: ";
  id += str.c_str();
  return id;
}

void YosysConverterModel2::addCell(CellID &cell) {
  stackNetBuilder.back().addCell(cell);
}

void YosysConverterModel2::insertEntityLinks(
    int entity, int port, CellID idCell) {
  curModule.entitiesLinks.emplace(entity, LinkEnd(idCell, port));
}

void YosysConverterModel2::walkWires(const IdDict<RTLIL::Wire *> &ywires) {
  for (const auto &[str, ywire]: ywires) {
    bool portOutput = ywire->port_output;
    bool portInput = ywire->port_input;
    unsigned index = str.index_;
    Mode mode;
    std::string modeStr;
    if (portInput) {
      modeStr = "input";
      mode = M_Input;
    }
    if (portOutput) {
      mode = M_Output;
      modeStr = "output";
    }
    if (!portOutput && !portInput) {
      mode = M_Wire;
      modeStr = "wire";
    }
    insertModeData(index, mode);
    debug << fmt::format("  index: {}  width: {}\n mode: {}",
        index, std::to_string(ywire->width), modeStr);
  }
}

void YosysConverterModel2::insertModeData(int idWire, Mode mode) {
  curModule.entitiesMode.emplace(idWire, mode);
}

std::vector<int> YosysConverterModel2::deterConst(
    const RTLIL::SigChunk &sigWire) {
  int idWire;
  CellID bit;
  std::vector<int> vecIdWire;
  const std::vector<RTLIL::State> &bits = sigWire.data;
  for (const auto &state: bits) {
    if (state == 0) {
      bit = UModel::makeCell(UModel::ZERO);
    } else if (state == 1) {
      bit = UModel::makeCell(UModel::ONE);
    } else if (state == 2) {
      bit = UModel::makeCell(UModel::ZERO);
      debug << "\nX_VALUE\n";
    } else {
      assert(0 && "Unsupported const");
    }
    addCell(bit);
    idWire = getNewIndex();
    insertEntityLinks(idWire, 0, bit);
    insertModeData(idWire, M_Const);
    vecIdWire.push_back(idWire);
  }
  return vecIdWire;
}

std::vector<int> YosysConverterModel2::deterSigSpecBit(
    const RTLIL::SigChunk &chunk) {
  std::vector<int> vecIdWire;
  const RTLIL::Wire *sig = chunk.wire;
  if (sig != nullptr) {
    int idWire = sig->name.index_;
    vecIdWire.push_back(idWire);
  } else {
    vecIdWire = deterConst(chunk);
  }
  return vecIdWire;
}

void YosysConverterModel2::appendVec(
    std::vector<int> &target, const std::vector<int> &source) {
  target.insert(target.end(), source.begin(), source.end());
}

std::vector<int> YosysConverterModel2::deterSigSpec(
    const RTLIL::SigSpec &sigWire) {
  std::vector<int> vecIdWire;
  const std::vector<RTLIL::SigChunk> &chunks = sigWire.chunks();
  for (const auto &elem: chunks) {
    std::vector<int> chunk = deterSigSpecBit(elem);
    appendVec(vecIdWire, chunk);
  }
  assert(vecIdWire.size() && "Not registred SigSpec");
  return vecIdWire;
}

std::vector<int> YosysConverterModel2::combineVectors(
    const std::vector<std::vector<int>> &vectors) {
  std::vector<int> combined_vector;
  size_t total_size = 0;
  for (const auto &vec : vectors) {
    total_size += vec.size();
  }
  combined_vector.reserve(total_size);
  for (const auto &vec : vectors) {
    std::copy(vec.begin(), vec.end(), std::back_inserter(combined_vector));
  }
  return combined_vector;
}

std::vector<std::uint16_t> YosysConverterModel2::combineVectorsSize(
    const std::vector<std::vector<int>> &vectors) {
  std::vector<std::uint16_t> combined_vector;
  for (const auto &vec : vectors) {
    combined_vector.push_back(vec.size());
  }
  return combined_vector;
}

void YosysConverterModel2::makeRestCell(
    int indexOperator, std::vector<int> &q, std::vector<std::vector<int>> &leafs, bool sign) {
  std::vector<int> linearLeafs = combineVectors(leafs);
  std::vector<std::uint16_t> linearLeafsSize = combineVectorsSize(leafs);
  std::vector<std::uint16_t> linearQSize = { static_cast<unsigned short>(q.size()) };
  makeSoftOperatorCell(
      indexOperator, q, linearLeafs, linearLeafsSize, linearQSize, sign);
}

void YosysConverterModel2::makeDFF(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, q;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { clk, data };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeDFFSR(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, set, clr, q;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_SET) {
      set = findings;
    } else if (namePort == SID_CLR) {
      clr = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  if (q.size() != data.size()) {
    assert(0 && "Not supported operator");
  }
  std::vector<std::vector<int>> leafs = { clk, data, set, clr };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeSR(const RTLIL::Cell *cell) {
  std::vector<int> set, clr, q;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == "SET") {
      set = findings;
    } else if (namePort == SID_CLR) {
      clr = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { set, clr };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeALDFF(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, q, aload;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_ALOAD) {
      aload = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { clk, aload, data };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeALDFFE(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, q, aload, en;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_ALOAD) {
      aload = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_EN) {
      en = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { clk, aload, en, data };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeDlatch(const RTLIL::Cell *cell) {
  std::vector<int> en, data, q;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_EN) {
      en = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { en, data };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeDlatchsr(const RTLIL::Cell *cell) {
  std::vector<int> en, data, set, clr, q;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_EN) {
      en = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_SET) {
      set = findings;
    } else if (namePort == SID_CLR) {
      clr = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { en, data, set, clr };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeADlatch(const RTLIL::Cell *cell) {
  std::vector<int> en, data, q, rst, value;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_EN) {
      en = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_ARST) {
      rst = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  value = determineInitValue(cell->parameters);
  std::vector<std::vector<int>> leafs = { en, rst, data, value };
  makeRestCell(cell->type.index_, q, leafs);
}

std::vector<int> YosysConverterModel2::determineInitValue(
    const IdDict<RTLIL::Const> &parms) {
  for (const auto &[str, mean]: parms) {
    if (str.str() == SID_ARST_VALUE) {
      return deterConst(mean);
    }
  }
  assert(0 && "Unsupported format parameters of dff cell");
  return { 0 };
}

void YosysConverterModel2::makeADFF(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, q, rst, value;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_ARST) {
      rst = findings;
    } else {
      assert(0 && "Unsupported format adff cells\n");
    }
  }
  value = determineInitValue(cell->parameters);
  std::vector<std::vector<int>> leafs = { clk, rst, data, value };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeADFFE(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, q, rst, value, en;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_ARST) {
      rst = findings;
    } else if (namePort == SID_EN) {
      en = findings;
    } else {
      assert(0 && "Unsupported format adff cells\n");
    }
  }
  value = determineInitValue(cell->parameters);
  std::vector<std::vector<int>> leafs = { clk, rst, en, data, value };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeDFFE(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, q, en;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_EN) {
      en = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { clk, en, data };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeDFFSRE(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, q, en, set, clr;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_EN) {
      en = findings;
    } else if (namePort == SID_SET) {
      set = findings;
    } else if (namePort == SID_CLR) {
      clr = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { clk, en, data, set, clr };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeSDFFE(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, q, en, srst;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_SRST) {
      srst = findings;
    } else if (namePort == SID_EN) {
      en = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { clk, en, srst, data };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeSDFFCE(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, q, en, srst;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_EN) {
      en = findings;
    } else if (namePort == SID_SRST) {
      srst = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { clk, en, srst, data };
  makeRestCell(cell->type.index_, q, leafs);
}

void YosysConverterModel2::makeSDFF(const RTLIL::Cell *cell) {
  std::vector<int> clk, data, q, srst;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_CLK) {
      clk = findings;
    } else if (namePort == SID_D) {
      data = findings;
    } else if (namePort == SID_Q) {
      q = findings;
    } else if (namePort == SID_SRST) {
      srst = findings;
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  std::vector<std::vector<int>> leafs = { clk, srst, data };
  makeRestCell(cell->type.index_, q, leafs);
}

YosysConverterModel2::CellSymbol YosysConverterModel2::getCellSymbolOperator(
    Operator op, bool sign) {
  CellSymbol symbol = UModel::UNDEF;
  if (!sign) {
    switch (op) {
    case O_Mul: symbol = UModel::MULu; break;
    case O_Div: symbol = UModel::DIVu; break;
    case O_Gt: symbol = UModel::GTu; break;
    case O_Geq: symbol = UModel::GTEu; break;
    case O_Leq: symbol = UModel::LTEu; break;
    case O_Lt: symbol = UModel::LTu; break;
    case O_Neq: symbol = UModel::NEQu; break;
    case O_Eq: symbol = UModel::EQu; break;
    default: break;
    }
  } else {
    switch (op) {
    case O_Mul: symbol = UModel::MULs; break;
    case O_Div: symbol = UModel::DIVs; break;
    case O_Gt: symbol = UModel::GTs; break;
    case O_Geq: symbol = UModel::GTEs; break;
    case O_Leq: symbol = UModel::LTEs; break;
    case O_Lt: symbol = UModel::LTs; break;
    case O_Neq: symbol = UModel::NEQs; break;
    case O_Eq: symbol = UModel::EQs; break;
    default: break;
    }
  }
  if (symbol == UModel::UNDEF) {
    switch (op) {
    case O_Not: symbol = UModel::NOT; break;
    case O_And: symbol = UModel::AND; break;
    case O_Or: symbol = UModel::OR; break;
    case O_Xor: symbol = UModel::XOR; break;
    case O_Xnor: symbol = UModel::XNOR; break;
    case O_Nor: symbol = UModel::NOR; break;
    case O_Nand: symbol = UModel::NAND; break;
    case O_Dff: symbol = UModel::DFF; break;
    case O_Dffrs: symbol = UModel::DFFrs; break;
    case O_Dlatch: symbol = UModel::LATCH; break;
    case O_Pos: symbol = UModel::BUF; break;
    case O_Mux: symbol = UModel::MUX2; break;
    case O_Mod: symbol = UModel::REMs; break;
    case O_Add: symbol = UModel::ADD; break;
    case O_Sub: symbol = UModel::SUB; break;
    default: symbol = UModel::UNDEF; break;
    }
  }
  return symbol;
}

YosysConverterModel2::CellTypeID YosysConverterModel2::getCellType(
    YosysConverterModel2::Operator op, YosysConverterModel2::Attributes *attr, bool sign) {
  CellSymbol symbol = getCellSymbolOperator(op, sign);
  return UModel::makeSoftType(
      symbol, operatorToString(op), eda::gate::model::OBJ_NULL_ID, attr->widthIn, attr->widthOut);
}

YosysConverterModel2::Signal *YosysConverterModel2::makeSignal(
    std::vector<int> &chunks) {
  Signal *newSig = new Signal;
  newSig->chunks = chunks;
  signals.push_back(newSig);
  return newSig;
}

void YosysConverterModel2::fillOperatorData(
    int typeFunction, YosysConverterModel2::Signal *lhs, YosysConverterModel2::Attributes *attrs, bool sign) {
  Operator operator_ = logicFunction(typeFunction);
  CellTypeID typeId = getCellType(operator_, attrs, sign);
  Operators op_(typeId, operator_);
  insertOperatorData(lhs, &op_);
}

void YosysConverterModel2::insertYosysCells(
    YosysConverterModel2::Signal *lhs, YosysConverterModel2::Signal *rhs) {
  curModule.yosysCells.emplace(lhs, rhs);
}

void YosysConverterModel2::makeSoftOperatorCell(
    int typeFunction, std::vector<int> &root, std::vector<int> &leaf, std::vector<std::uint16_t> &widthIn, std::vector<std::uint16_t> &widthOut, bool sign) {
  Signal *lhs = makeSignal(root);
  Signal *rhs = makeSignal(leaf);
  auto attr = Attributes(widthIn, widthOut);
  insertYosysCells(lhs, rhs);
  fillOperatorData(typeFunction, lhs, &attr, sign);
}

void YosysConverterModel2::makeConnectArnity1(
    const int typeFunction, const IdDict<RTLIL::SigSpec> &cons) {
  std::vector<int> root, leaf;
  const IdDict<RTLIL::SigSpec> &cons_ = cons;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_Y) {
      root = findings;
    } else if (namePort == SID_A) {
      leaf = findings;
    } else {
      assert(0 && "Unsupported format unarny cell");
    }
  }
  std::vector<std::vector<int>> leafs = { leaf };
  makeRestCell(typeFunction, root, leafs);
}

void YosysConverterModel2::makeConnectArnity2(const RTLIL::Cell *cell) {
  int typeFunction = cell->type.index_;
  std::vector<int> root, A, B;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::string namePort = str.str();
    std::vector<int> findings = deterSigSpec(sig);
    if (namePort == SID_Y) {
      root = findings;
    } else if (namePort == SID_A) {
      A = findings;
    } else if (namePort == SID_B) {
      B = findings;
    } else {
      assert(0 && "Unsupported format binarny cell");
    }
  }
  bool sign = false;
  const IdDict<RTLIL::Const> &parms = cell->parameters;
  for (const auto &[str, constant]: parms) {
    std::string nameParm = str.str();
    if (nameParm == SID_A_SIGNED) {
      constant[0] == 1 ? sign = true : sign = false;
    }
  }
  std::vector<std::vector<int>> leafs = { A, B };
  makeRestCell(typeFunction, root, leafs, sign);
}

void YosysConverterModel2::printCell(const RTLIL::Cell *cell) {
  std::string connectStr;
  const IdDict<RTLIL::SigSpec> &co = cell->connections_;
  for (const auto &[str, sig]: co) {
    connectStr += fmt::format("   {} index: {} : {}\n",
        readIdString(str), std::to_string(str.index_), Yosys::log_signal(sig));
  }
  std::string parameterStr;
  const IdDict<RTLIL::Const> &pars = cell->parameters;
  for (const auto &[str, constant]: pars) {
    parameterStr += fmt::format("   {} index: {} : {}\n",
        readIdString(str), std::to_string(str.index_), Yosys::log_signal(constant));
  }
  debug << fmt::format(" Connections:\n {}\n Parameters:\n {}\n",
      connectStr, parameterStr);
}

void YosysConverterModel2::makeMux(const RTLIL::Cell *cell) {
  std::vector<int> root, signal, A, B;
  const IdDict<RTLIL::SigSpec> &cons_ = cell->connections_;
  for (const auto &[str, sig]: cons_) {
    std::vector<int> findings = deterSigSpec(sig);
    std::string namePort = str.str();
    if (namePort == SID_Y) {
      root = findings;
    } else if (namePort == SID_S) {
      signal = findings;
    } else if (namePort == SID_A) {
      A = findings;
    } else if (namePort == SID_B) {
      B = findings;
    } else {
      assert(0 && "Unsupported format mux cells");
    }
  }
  std::vector<std::vector<int>> leafs = { signal, A, B };
  makeRestCell(cell->type.index_, root, leafs);
}

bool YosysConverterModel2::isMux(int index) {
  return (index == ID($mux).index_) || (index == ID($ternary).index_) || (index == ID($pmux).index_);
}

bool YosysConverterModel2::isDFF(int index) {
  return index == ID($dff).index_;
}

bool YosysConverterModel2::isDFFSR(int index) {
  return index == ID($dffsr).index_;
}

bool YosysConverterModel2::isDlatch(int index) {
  return index == ID($dlatch).index_;
}

bool YosysConverterModel2::isADlatch(int index) {
  return index == ID($adlatch).index_;
}

bool YosysConverterModel2::isDlatchsr(int index) {
  return index == ID($dlatchsr).index_;
}

bool YosysConverterModel2::isInitCell(int typeFunction) {
  bool isInit;
  if (modulesMap.find(typeFunction) != modulesMap.end()) {
    isInit = true;
  } else {
    isInit = false;
  }
  return isInit;
}

void YosysConverterModel2::insertOperatorData(Signal *sig, Operators *op) {
  curModule.operators.emplace(sig, *op);
}

void YosysConverterModel2::gatherPortsInfo(
    YosysConverterModel2::PortsInfo &portsInfo, std::vector<int> &root, const RTLIL::Cell *cell) {
  const IdDict<RTLIL::SigSpec> &cons = cell->connections_;
  for (const auto &[str, sig]: cons) {
    std::string nestedModuleNamePort = str.str();
    std::vector<int> theseModulePort = deterSigSpec(sig);
    portsInfo.ports.emplace(nestedModuleNamePort, theseModulePort[0]);
    root.push_back(theseModulePort[0]);
  }
}

void YosysConverterModel2::fillInitCellData(
    RTLIL::Module *nested, PortsInfo *portsInfo) {
  std::vector<std::uint16_t> wIn, wOut;
  for (size_t i = 0; i < portsInfo->orderInputs.size(); i++) {
    wIn.push_back(1);
  }
  for (size_t i = 0; i < portsInfo->orderOutputs.size(); i++) {
    wOut.push_back(1);
  }
  CellTypeID typeInst = UModel::makeSoftType(
      UModel::UNDEF, nested->name.c_str(), listNetID.back(), wIn, wOut);
  listNetID.pop_back();
  Operators op;
  op.operator_ = O_Init;
  op.cellTypeId = typeInst;
  Signal *lhs = makeSignal(portsInfo->orderOutputs);
  Signal *rhs = makeSignal(portsInfo->orderInputs);
  insertOperatorData(lhs, &op);
  insertYosysCells(lhs, rhs);
}

void YosysConverterModel2::walkNestedModule(RTLIL::Module *nested) {
  Subnet copyCurModule = curModule;
  Subnet defaultModule;
  curModule = defaultModule;
  walkModule(nested);
  curModule = copyCurModule;
}

void YosysConverterModel2::makeInitCells(const RTLIL::Cell *cell) {
  PortsInfo portsInfo;
  std::vector<int> root;
  gatherPortsInfo(portsInfo, root, cell);
  int indexModule = cell->type.index_;
  RTLIL::Module *nested = modulesMap[indexModule];
  gatherModeInfo(portsInfo, nested);
  walkNestedModule(nested);
  fillInitCellData(nested, &portsInfo);
}

bool YosysConverterModel2::isADFF(int type) {
  return type == ID($adff).index_;
}

bool YosysConverterModel2::isADFFE(int type) {
  return type == ID($adffe).index_;
}

bool YosysConverterModel2::isDFFE(int type) {
  return type == ID($dffe).index_;
}

bool YosysConverterModel2::isDFFSRE(int type) {
  return type == ID($dffsre).index_;
}

bool YosysConverterModel2::isSDFFE(int type) {
  return type == ID($sdffe).index_;
}

bool YosysConverterModel2::isSDFF(int type) {
  return type == ID($sdff).index_;
}

bool YosysConverterModel2::isALDFF(int type) {
  return type == ID($aldff).index_;
}

bool YosysConverterModel2::isALDFFE(int type) {
  return type == ID($aldffe).index_;
}

bool YosysConverterModel2::isSR(int type) {
  return type == ID($sr).index_;
}

bool YosysConverterModel2::isSDFFCE(int type) {
  return type == ID($sdffce).index_;
}

void YosysConverterModel2::makeTriggerOrConnectAnity2(const RTLIL::Cell *cell) {
  int typeFunction = cell->type.index_;
  if (isDFF(typeFunction)) {
    makeDFF(cell);
  } else if (isDlatch(typeFunction)) {
    makeDlatch(cell);
  } else if (isSR(typeFunction)) {
    makeSR(cell);
  } else {
    makeConnectArnity2(cell);
  }
}

void YosysConverterModel2::makeConnectArnity3(const RTLIL::Cell *cell) {
  int typeFunction = cell->type.index_;
  if (isMux(typeFunction)) {
    makeMux(cell);
  } else if (isADFF(typeFunction)) {
    makeADFF(cell);
  } else if (isADFFE(typeFunction)) {
    makeADFFE(cell);
  } else if (isDFFE(typeFunction)) {
    makeDFFE(cell);
  } else if (isSDFFE(typeFunction)) {
    makeSDFFE(cell);
  } else if (isSDFF(typeFunction)) {
    makeSDFF(cell);
  } else if (isALDFFE(typeFunction)) {
    makeALDFFE(cell);
  } else if (isALDFF(typeFunction)) {
    makeALDFF(cell);
  } else if (isADlatch(typeFunction)) {
    makeADlatch(cell);
  } else if (isDlatchsr(typeFunction)) {
    makeDlatchsr(cell);
  } else if (isDFFSR(typeFunction)) {
    makeDFFSR(cell);
  } else if (isDFFSRE(typeFunction)) {
    makeDFFSRE(cell);
  } else if (isSDFFCE(typeFunction)) {
    makeSDFFCE(cell);
  } else {
    assert(0 && "Not supported ternary operator");
  }
}

void YosysConverterModel2::printDataCell(
    const RTLIL::IdString &str, const RTLIL::Cell *cell) {
  debug << fmt::format(" Cell: {} index: {} type: {}\n",
      str.c_str(), str.index_, cell->type.c_str());
}

void YosysConverterModel2::walkCells(const IdDict<RTLIL::Cell *> &ycells) {
  for (const auto &[str, cell]: ycells) {
    printDataCell(str, cell);
    printCell(cell);
    int typeFunction = cell->type.index_;
    if (isInitCell(typeFunction)) {
      makeInitCells(cell);
      continue;
    }
    Operator op = logicFunction(typeFunction);
    int arnity = determineTypeOperator(op);
    if (arnity == 1) {
      makeConnectArnity1(typeFunction, cell->connections_);
    } else if (arnity == 2) {
      makeTriggerOrConnectAnity2(cell);
    } else if (arnity == 3) {
      makeConnectArnity3(cell);
    }
  }
}

bool YosysConverterModel2::isUnOperator(Operator func) {
  switch (func) {
  case O_Not:
  case O_Orr:
  case O_Xorr:
  case O_Boolr:
  case O_Xnorr:
  case O_Andr:
  case O_Neg:
  case O_Pos:
    return true;
  default:
    return false;
  }
}

bool YosysConverterModel2::isBinOperator(Operator func) {
  switch (func) {
  case O_Add:
  case O_Sub:
  case O_Or:
  case O_And:
  case O_Xor:
  case O_Mul:
  case O_Div:
  case O_Cat:
  case O_Shl:
  case O_Shr:
  case O_Dshl:
  case O_Dshr:
  case O_Pad:
  case O_Geq:
  case O_Gt:
  case O_Lt:
  case O_Leq:
  case O_Neq:
  case O_Eq:
  case O_Eqx:
  case O_Nex:
  case O_Pow:
  case O_Mod:
  case O_Divfloor:
  case O_Modfloor:
  case O_Nand:
  case O_Xnor:
  case O_Nor:
  case O_Dff:
  case O_Dlatch:
  case O_Sr:
    return true;
  default:
    return false;
  }
}

bool YosysConverterModel2::isTernOperator(Operator func) {
  switch (func) {
  case O_Mux:
  case O_Pmux:
  case O_Dffrs:
  case O_Dffe:
  case O_Sdff:
  case O_Sdffe:
  case O_Sdffce:
  case O_Adffe:
  case O_Aldffe:
  case O_Aldff:
  case O_Adlatch:
  case O_Dlatchsr:
  case O_Dffsr:
  case O_Dffsre:
    return true;
  default:
    return false;
  }
}

int YosysConverterModel2::determineTypeOperator(Operator func) {
  int arity = 0;
  if (isUnOperator(func)) {
    arity = 1;
  }
  if (isBinOperator(func)) {
    arity = 2;
  }
  if (isTernOperator(func)) {
    arity = 3;
  }
  assert(arity && "Problem with determine type of operator");
  return arity;
}

YosysConverterModel2::CellTypeID
YosysConverterModel2::getOpTypeId(Signal *lhs) {
  return curModule.operators[lhs].cellTypeId;
}

YosysConverterModel2::Operator YosysConverterModel2::getOperator(Signal *lhs) {
  return curModule.operators[lhs].operator_;
}

YosysConverterModel2::LinkList
YosysConverterModel2::makeEmptyLinkList(size_t length) {
  LinkList list;
  for (size_t i = 0; i < length; i++) {
    list.push_back(LinkEnd());
  }
  return list;
}

YosysConverterModel2::LinkEnd
YosysConverterModel2::getEntityLinks(int entity) {
  return curModule.entitiesLinks[entity];
}

void YosysConverterModel2::insertCellCompliance(
    YosysConverterModel2::Signal *lhs, YosysConverterModel2::CellID idCell) {
  curModule.cellCompliance.emplace(lhs, idCell);
}

YosysConverterModel2::CellID
YosysConverterModel2::getCellCompliance(Signal *lhs) {
  return curModule.cellCompliance[lhs];
}

void YosysConverterModel2::buildEmptyCells() {
  std::map<Signal *, Signal *> &cells = curModule.yosysCells;
  for (const auto &[lhs, rhs]: cells) {
    CellTypeID idOperator = getOpTypeId(lhs);
    LinkList list = makeEmptyLinkList(rhs->chunks.size());
    CellID idCell = UModel::makeCell(idOperator, list);
    insertCellCompliance(lhs, idCell);
    addCell(idCell);
    std::vector<int> &chunks = lhs->chunks;
    for (size_t i = 0; i < chunks.size(); i++) {
      insertEntityLinks(chunks[i], static_cast<int>(i), idCell);
    }
  }
}

YosysConverterModel2::Mode YosysConverterModel2::getMode(int entity) {
  return curModule.entitiesMode[entity];
}

void YosysConverterModel2::makeConnectionsCells() {
  std::map<Signal *, Signal *> &cells = curModule.yosysCells;
  for (const auto &[lhs, rhs]: cells) {
    CellID idCell = getCellCompliance(lhs);
    std::vector<int> &chunks = rhs->chunks;
    for (size_t i = 0; i < chunks.size(); i++) {
      stackNetBuilder.back().connect(
          idCell, static_cast<int>(i), getEntityLinks(chunks[i]));
    }
  }
}

void YosysConverterModel2::walkYosysCells() {
  buildEmptyCells();
  makeConnectionsCells();
}

void YosysConverterModel2::walkConnections(
    const std::vector<std::pair<RTLIL::SigSpec, RTLIL::SigSpec>> &cons) {
  walkYosysCells();
  for (const auto &[op1, op2]: cons) {
    printConnections(op1, op2);

    std::vector<int> index1 = deterSigSpec(op1);
    std::vector<int> index2 = deterSigSpec(op2);

    if (index1.size() != index2.size()) {
      assert(0 && "Unsupported format connections statement");
    }

    for (size_t i = 0; i < index2.size(); ++i) {
      CellID cell = UModel::makeCell(UModel::BUF, getEntityLinks(index2[i]));
      addCell(cell);
      insertEntityLinks(index1[i], 0, cell);
    }
  }
}

void YosysConverterModel2::insertPortMode(
    YosysConverterModel2::PortsInfo &pInfo, int port, Mode mode) {
  pInfo.portsMode.emplace(port, mode);
}

void YosysConverterModel2::gatherModeInfo(
    YosysConverterModel2::PortsInfo &pInfo, RTLIL::Module *nested) {
  const IdDict<RTLIL::Wire *> &ywires = nested->wires_;
  for (const auto &[str, ywire]: ywires) {
    bool portOutput = ywire->port_output;
    bool portInput = ywire->port_input;
    std::string name = str.str();
    std::map<std::string, int> &portsInfo = pInfo.ports;
    if (portInput) {
      int in = portsInfo[name];
      insertPortMode(pInfo, in, M_Input);
    } else if (portOutput) {
      int out = portsInfo[name];
      insertPortMode(pInfo, out, M_Output);
    } else if (portsInfo.find(name) != portsInfo.end()) {
      assert(0 && "Error in gatherModeInfo");
    }
  }
  const std::vector<RTLIL::IdString> &ports = nested->ports;
  for (const auto &port: ports) {
    int index = port.index_;
    Mode mode = pInfo.portsMode[index];
    if (mode == M_Input) {
      pInfo.orderInputs.push_back(index);
    } else if (mode == M_Output) {
      pInfo.orderOutputs.push_back(index);
    } else {
      assert(0 && "Error with init cell port");
    }
  }
}

void YosysConverterModel2::printConnections(
    const RTLIL::SigSpec &op1, const RTLIL::SigSpec &op2) {
  debug << fmt::format(" Connect:\n  1st operand {} size: {}\n  2nd operand {} size: {}\n",
      Yosys::log_signal(op1), op1.chunks().size(), Yosys::log_signal(op2), op2.chunks().size());
}

void YosysConverterModel2::walkParameteres(
    const Yosys::hashlib::idict<RTLIL::IdString> &avail_parameters) {
  for (const auto &parameter: avail_parameters) {
    debug << fmt::format(" index: {} name: {}\n",
        parameter.index_, parameter.c_str());
  }
}

void YosysConverterModel2::walkPorts(
    const std::vector<RTLIL::IdString> &ports, bool buildOut = false) {
  for (const auto &port: ports) {
    int index = port.index_;
    Mode mode = getMode(index);
    CellID idCell;
    if (mode == M_Input && !buildOut) {
      idCell = UModel::makeCell(UModel::IN);
      addCell(idCell);
      insertEntityLinks(index, 0, idCell);
    } else if (mode == M_Output && buildOut) {
      CellID out = UModel::makeCell(UModel::OUT, getEntityLinks(index));
      addCell(out);
    }
    debug << fmt::format(" {} index: {}\n", readIdString(port), port.index_);
  }
}

void YosysConverterModel2::walkMemories(
    const IdDict<RTLIL::Memory *> &processes) {
  for (const auto &[str, memory]: processes) {
    debug << fmt::format("  {} index: {} width: {} start_offset: {} size: {}\n",
        readIdString(str), str.index_, memory->width, memory->start_offset, memory->size);
  }
}

void YosysConverterModel2::walkModule(const Yosys::RTLIL::Module *m) {
  debug << "Start walking\n";
  NetBuilder curNetBuilder;
  stackNetBuilder.push_back(curNetBuilder);
  debug << "Wires:\n";
  walkWires(m->wires_);
  debug << "End Wires\n\n";
  debug << "Ports:\n";
  walkPorts(m->ports, false);
  debug << "End Ports\n\n";
  debug << "Memories\n";
  walkMemories(m->memories);
  debug << "End Memories\n";
  debug << "Cells:\n\n";
  walkCells(m->cells_);
  debug << "End Cells\n\n";
  debug << "Connections:\n";
  walkConnections(m->connections_);
  debug << "End Connections\n\n";
  debug << "Ports:\n";
  walkPorts(m->ports, true);
  debug << "End Ports\n\n";
  debug << "Avail parameters:\n";
  walkParameteres(m->avail_parameters);
  debug << "End Avail parameteres\n\n";
  UModel::NetID idNet = curNetBuilder.make();
  const Net &net = Net::get(idNet);
  debug << net << "\n";
  listNetID.push_back(idNet);
  stackNetBuilder.pop_back();
}

YosysConverterModel2::Operator YosysConverterModel2::logicFunction(int type) {
  if (type == ID($add).index_) {
    return O_Add;
  }
  if (type == ID($sub).index_) {
    return O_Sub;
  }
  if (type == ID($and).index_ ||
      type == ID($logic_and).index_) {
    return O_And;
  }
  if (type == ID($nand).index_) {
    return O_Nand;
  }
  if (type == ID($xnor).index_) {
    return O_Xnor;
  }
  if (type == ID($nor).index_) {
    return O_Nor;
  }
  if (type == ID($or).index_ ||
      type == ID($logic_or).index_) {
    return O_Or;
  }
  if (type == ID($reduce_or).index_) {
    return O_Orr;
  }
  if (type == ID($reduce_and).index_) {
    return O_Andr;
  }
  if (type == ID($reduce_xor).index_) {
    return O_Xorr;
  }
  if (type == ID($reduce_xnor).index_) {
    return O_Xnorr;
  }
  if (type == ID($xor).index_) {
    return O_Xor;
  }
  if (type == ID($shl).index_) {
    return O_Shl;
  }
  if (type == ID($shr).index_) {
    return O_Shr;
  }
  if (type == ID($sshl).index_) {
    return O_Dshl;
  }
  if (type == ID($sshr).index_) {
    return O_Dshr;
  }
  if (type == ID($not).index_ ||
      type == ID($logic_not).index_) {
    return O_Not;
  }
  if (type == ID($reduce_bool).index_) {
    return O_Boolr;
  }
  if (type == ID($le).index_) {
    return O_Leq;
  }
  if (type == ID($lt).index_) {
    return O_Lt;
  }
  if (type == ID($ge).index_) {
    return O_Geq;
  }
  if (type == ID($gt).index_) {
    return O_Gt;
  }
  if (type == ID($mul).index_) {
    return O_Mul;
  }
  if (type == ID($concat).index_) {
    return O_Cat;
  }
  if (type == ID($mux).index_ ||
      type == ID($ternary).index_) {
    return O_Mux;
  }
  if (type == ID($pmux).index_) {
    return O_Pmux;
  }
  if (type == ID($div).index_) {
    return O_Div;
  }
  if (type == ID($neg).index_) {
    return O_Neg;
  }
  if (type == ID($pos).index_) {
    return O_Pos;
  }
  if (type == ID($ne).index_) {
    return O_Neq;
  }
  if (type == ID($eq).index_) {
    return O_Eq;
  }
  if (type == ID($eqx).index_) {
    return O_Eqx;
  }
  if (type == ID($nex).index_) {
    return O_Nex;
  }
  if (type == ID($pow).index_) {
    return O_Pow;
  }
  if (type == ID($mod).index_) {
    return O_Mod;
  }
  if (type == ID($divfloor).index_) {
    return O_Divfloor;
  }
  if (type == ID($modfloor).index_) {
    return O_Modfloor;
  }
  if (type == ID($pos).index_) {
    return O_Pad;
  }
  if (type == ID($dff).index_) {
    return O_Dff;
  }
  if (type == ID($dffsr).index_) {
    return O_Dffsr;
  }
  if (type == ID($adffe).index_) {
    return O_Adffe;
  }
  if (type == ID($aldff).index_) {
    return O_Aldff;
  }
  if (type == ID($aldffe).index_) {
    return O_Aldffe;
  }
  if (type == ID($adff).index_) {
    return O_Dffrs;
  }
  if (type == ID($dffe).index_) {
    return O_Dffe;
  }
  if (type == ID($dffsre).index_) {
    return O_Dffsre;
  }
  if (type == ID($sdffe).index_) {
    return O_Sdffe;
  }
  if (type == ID($sdff).index_) {
    return O_Sdff;
  }
  if (type == ID($dlatch).index_) {
    return O_Dlatch;
  }
  if (type == ID($dlatchsr).index_) {
    return O_Dlatchsr;
  }
  if (type == ID($adlatch).index_) {
    return O_Adlatch;
  }
  if (type == ID($sdffce).index_) {
    return O_Sdffce;
  }
  if (type == ID($sr).index_) {
    return O_Sr;
  }
  assert(false && "Unsupport operator ");
  return O_Not;
}
