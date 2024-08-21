//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "yosys_converter_firrtl.h"

#include <fmt/ostream.h>

#include <sstream>

namespace RTLIL = Yosys::RTLIL;

std::ostream YosysConverterFirrtl::devnull(nullptr);

YosysConverterFirrtl::YosysConverterFirrtl(const FirrtlConfig &config)
    : outputFile(config.outputFileName.empty() ? std::cout : file)
    , debug(config.debugMode ? std::cerr : devnull) {
  Yosys::yosys_setup();
  RTLIL::Design design;
  std::string files;
  for (const auto &file: config.files) {
    files += file + " ";
  }
  files.pop_back();
  Yosys::run_pass("design -reset-vlog", &design);
  std::string command = "read_verilog " + files;
  Yosys::run_pass(command, &design);
  deterTopModule(design, config.topModule);
  Yosys::run_pass("proc", &design);
  Yosys::run_pass("opt -nodffe -nosdff", &design);
  Yosys::run_pass("memory", &design);
  Yosys::run_pass("opt -nodffe -nosdff", &design);
  Yosys::run_pass("pmuxtree", &design);
  createOutputFile(config.outputFileName);
  readModules(design);
}

void YosysConverterFirrtl::deterTopModule(
    RTLIL::Design &design, const std::string &topModule) {
  if (topModule.empty()) {
    Yosys::run_pass("hierarchy -auto-top", &design);
    RTLIL::Module *module = design.top_module();
    nameTopModule = module->name.c_str();
    nameTopModule.erase(0, 1);
  } else {
    nameTopModule = topModule;
  }
}

void YosysConverterFirrtl::createOutputFile(const std::string &outputNamefile) {
  if (!outputNamefile.empty()) {
    file.open(outputNamefile, std::ios::out | std::ios::binary);
    file.exceptions(std::ios::failbit | std::ios::badbit);
  }
}

void YosysConverterFirrtl::declareSignal(
    std::ostream &os, const Signal &sig) {
  if (sig.isUsed && !sig.isConst) {
    std::string width;
    if (sig.width != -1 && sig.width != 0) {
      width = fmt::format("<{}>", sig.width);
    }
    os << fmt::format("    {} {} : {}{}",
        getPinModeName(sig.mode),
        sig.id,
        getTypeName(sig.type),
        width);
    if (sig.driverSig != "") {
      os << fmt::format(", {}", sig.driverSig);
    }
    if (sig.resetSig != "") {
      os << fmt::format(", {}, UInt({})", sig.resetSig, sig.resetMean);
    }
    os << "\n";
    if (sig.isInvalid && (sig.mode == PM_Wire)) {
      os << fmt::format("    invalidate {}\n", sig.id);
    }
  }
}

bool YosysConverterFirrtl::hasInOutMode(Signal *sig) {
  return sig->mode == PM_Input || sig->mode == PM_Output;
}

bool YosysConverterFirrtl::isDeclWire(Signal *sig) {
  return (sig->mode == PM_Wire) && sig->isDecl;
}

bool YosysConverterFirrtl::isReg(Signal *sig) {
  return (sig->mode == PM_Reg) || (sig->mode == PM_Regreset);
}

void YosysConverterFirrtl::declareMemory(
    std::ostream &os, const Memory &memory) {
  os << fmt::format("    mem {} :\n", memory.name);
  os << fmt::format("    data-type => UInt<{}> :\n", memory.widthData);
  os << fmt::format("    depth => {}\n", memory.depth);

  for (size_t i = 0; i < memory.controllers.size(); ++i) {
    std::string mode;
    if (memory.controllers[i].type == TC_Reader) {
      mode = "reader";
    } else {
      mode = "writer";
    }
    os << fmt::format("     {} => {}\n",
        mode,
        memory.controllers[i].name);
  }
  os << fmt::format("     read-latency => {}\n", memory.readLatency);
  os << fmt::format("     write-latency => {}\n", memory.writeLatency);
  os << "     read-under-write => undefined\n";
}

void YosysConverterFirrtl::declareModule(
    std::ostream &os, const Module &circuit) {
  os << "  module " << circuit.id << " :\n";
  const std::vector<int> &orderPorts = circuit.orderPorts;
  const std::map<int, Signal *> &circtSignals = circuit.signals;
  for (const auto &index: orderPorts) {
    if (circtSignals.find(index) != circtSignals.end()) {
      Signal *sig = circtSignals.at(index);
      if (hasInOutMode(sig)) {
        declareSignal(os, *sig);
      } else {
        assert(0 && "Problem with declaration of module: error with mode port");
      }
    } else {
      assert(0 && "Problem with declaration of module: signal isn't registred");
    }
  }
  for (const auto &[index, sig]: circtSignals) {
    if (isDeclWire(sig)) {
      declareSignal(os, *sig);
    }
  }
  for (const auto &[index, sig]: circtSignals) {
    if (isReg(sig)) {
      declareSignal(os, *sig);
    }
  }
  const std::map<int, Memory> &memories = circuit.memories;
  for (const auto &[id, mem]: memories) {
    declareMemory(os, mem);
  }
}

bool YosysConverterFirrtl::isMean(const std::string &mean) {
  size_t pos = mean.find("0b");
  if (pos != std::string::npos) {
    return true;
  }
  return false;
}

std::string YosysConverterFirrtl::getNameSignal(const Signal *sig) {
  std::string nameSignal;
  if (sig->isConst) {
    if (isMean(sig->mean)) {
      std::string width = std::to_string(sig->mean.size() - 2);
      nameSignal = fmt::format("UInt<{}>({})", width, sig->mean);
      return nameSignal;
    }
    return sig->mean;
  }
  if (sig == nullptr)
    return "";
  return sig->id;
}

std::string YosysConverterFirrtl::makeOperandPrint(const Operand &op) {
  std::string operand;
  if (op.hi != -1) {
    operand = fmt::format("bits({}, {}, {})",
        getNameSignal(op.sig),
        op.hi,
        op.lo);
  } else {
    operand = getNameSignal(op.sig);
  }
  return operand;
}

std::string YosysConverterFirrtl::makeNullary(const SigAssign &instr) {
  std::string rhs = makeOperandPrint(instr.op1);
  return rhs;
}

std::string YosysConverterFirrtl::makeUnary(const SigAssign &instr) {
  std::string rhs;
  std::string operand = makeOperandPrint(instr.op1);
  rhs += fmt::format("{}({})", operatorToString(instr.func), operand);
  if (instr.func == O_Neg) {
    rhs = fmt::format("asUInt({})", rhs);
  }
  return rhs;
}

bool YosysConverterFirrtl::isSpecifiedOperator(Operator func) {
  switch(func) {
  case O_Xnor:
  case O_Nor:
  case O_Nand:
    return true;
  default:
    return false;
  }
}

std::string YosysConverterFirrtl::makeBinary(const SigAssign &instr) {
  std::string rhs;
  std::string operand1 = makeOperandPrint(instr.op1);
  std::string operand2 = makeOperandPrint(instr.op2);
  rhs = fmt::format("{}({}, {})",
      operatorToString(instr.func),
      operand2,
      operand1);
  if (isSpecifiedOperator(instr.func)) {
    rhs = fmt::format("not({})", rhs);
  }
  return rhs;
}

std::string YosysConverterFirrtl::makeTernary(const SigAssign &instr) {
  std::string operand1 = makeOperandPrint(instr.op1);
  std::string operand2 = makeOperandPrint(instr.op2);
  std::string operand3 = makeOperandPrint(instr.op3);
  std::string rhs = fmt::format("{}({}, {}, {})",
      operatorToString(instr.func),
      operand1,
      operand2,
      operand3);
  return rhs;
}

void YosysConverterFirrtl::declareSigAssign(
    std::ostream &os, const SigAssign &instr) {
  std::string rhs;
  int arity = determineTypeOperator(instr.func);
  if (arity == 0) {
    rhs = makeNullary(instr);
  }
  if (arity == 1) {
    rhs = makeUnary(instr);
  }
  if (arity == 2) {
    rhs = makeBinary(instr);
  }
  if (arity == 3) {
    rhs = makeTernary(instr);
  }
  os << fmt::format("{} <= {}\n", getNameSignal(instr.lhs), rhs);
}

void YosysConverterFirrtl::declareConnectInstruction(
    std::ostream &os, const Instruction &instr) {
  const std::vector<SigAssign> &instrSA = instr.connects;
  for (const auto &sa: instrSA) {
    os << "    ";
    declareSigAssign(os, sa);
  }
}

void YosysConverterFirrtl::declareWhenInstruction(
    std::ostream &os, const Instruction &instr) {
  const std::vector<CondStatement> &instrCS = instr.branches;
  for (const auto &statement: instrCS) {
    countIndent(os, statement.branch.size());
    os << "    ";
    if (statement.branch.back() == CKW_If) {
      os << fmt::format("when {} :\n", getNameSignal(statement.sig));
    } else {
      os << "else :\n";
    }
    const std::vector<SigAssign> &stateSA = statement.connects;
    for (const auto &sa: stateSA) {
      os << "    ";
      countIndent(os, statement.branch.size());
      os << "  ";
      declareSigAssign(os, sa);
    }
    if (statement.connects.empty()) {
      os << "    ";
      countIndent(os, statement.branch.size());
      os << "  skip\n";
    }
  }
}

void YosysConverterFirrtl::declareInstInstruction(
    std::ostream &os, const Instruction &instr) {
  std::string id = instr.instance.idInstance;
  os << fmt::format("    inst {} of {}\n",
      id,
      instr.instance.nameNestedModule);
  const std::vector<DataPorts> &ports = instr.instance.ports;
  for (const auto &port: ports) {
    const std::pair<Signal *, Signal *> &op = port.signals;
    const std::pair<int, int> &parms = port.params;
    std::pair<int, std::string> key = { instr.instance.indexNestedModule, op.first->id };
    if (portsMode.find(key) == portsMode.end()) {
      debug << fmt::format("{} {}\nMap of ports: \n", key.first, key.second);
      for (auto it: portsMode) {
        debug << fmt::format("{} {}\n", it.first.first, it.first.second);
      }
      assert(0 && "Incorrect port data");
    }
    if (portsMode[key] == PM_Input) {
      os << fmt::format("    {}.{} <= ",
          id,
          getNameSignal(op.first));
      os << makeOperandPrint({ op.second, parms.first, parms.second });
    } else {
      os << fmt::format("    {} <= {}.{}",
          getNameSignal(op.second),
          id,
          getNameSignal(op.first));
    }
    os << "\n";
  }
}

void YosysConverterFirrtl::declareInstruction(
    std::ostream &os, const Instruction &instr) {
  if (instr.statement == S_Connect) {
    declareConnectInstruction(os, instr);
  }
  if (instr.statement == S_When) {
    declareWhenInstruction(os, instr);
  }
  if (instr.statement == S_Inst) {
    declareInstInstruction(os, instr);
  }
}

void YosysConverterFirrtl::declareInstructions(
    std::ostream &os, const std::vector<Instruction> &vec) {
  for (const auto &instr: vec) {
    declareInstruction(os, instr);
  }
}

YosysConverterFirrtl::~YosysConverterFirrtl() {
  outputFile << "FIRRTL version 3.2.0\n";
  outputFile << "circuit " << nameTopModule << " :\n";
  std::vector<Module> &modules = finalModules;
  for (const auto &curModule: modules) {

    declareModule(outputFile, curModule);
    declareInstructions(outputFile, curModule.instructions);

    const std::map<int, Signal *> &signals = curModule.signals;
    for (const auto &[index, sig]: signals) {
      delete sig;
    }

    const std::vector<Signal *> &genSignals = curModule.genSig;
    for (const auto &sig: genSignals) {
      delete sig;
    }
  }
}

std::string YosysConverterFirrtl::getName() {
  return genName + "_" + std::to_string(numbGenName++);
}

void YosysConverterFirrtl::countIndent(std::ostream &os, size_t indent) {
  for (size_t i = 0; i < indent - 1; ++i) {
    os << "  ";
  }
}

std::string YosysConverterFirrtl::operatorToString(Operator op) {
  switch (op) {
  case O_Add: return "add";
  case O_Sub: return "sub";
  case O_Mul: return "mul";
  case O_Div: return "div";
  case O_Not: return "not";
  case O_And: return "and";
  case O_Nand: return "not(and";
  case O_Or: return "or";
  case O_Nor: return "not(or";
  case O_Orr: return "orr";
  case O_Andr: return "andr";
  case O_Xorr: return "xorr";
  case O_Xor: return "xor";
  case O_Xnor: return "not(or";
  case O_Cat: return "cat";
  case O_Shl: return "shl";
  case O_Shr: return "shr";
  case O_Dshr: return "dshr";
  case O_Dshl: return "dshl";
  case O_Eq: return "eq";
  case O_Neq: return "neq";
  case O_Leq: return "leq";
  case O_Lt: return "lt";
  case O_Geq: return "geq";
  case O_Gt: return "gt";
  case O_Mux: return "mux";
  case O_Neg: return "neg";
  case O_Assign: return "assign";
  case O_Pad: return "pad";
  case O_AsClock: return "asClock";
  case O_AsAsyncReset: return "asAsyncReset";
  default:
    assert(0 && "unknown operator");
    return "unknown";
  }
}

void YosysConverterFirrtl::makeMapModules(
    const IdDict<RTLIL::Module *> &modulesYosys) {
  for (const auto &[str, module]: modulesYosys) {
    modules.emplace(str.index_, module);
    std::string nameModule = module->name.c_str();
    modulesName.emplace(str.index_, checkName(nameModule));
  }
}

void YosysConverterFirrtl::readModules(const RTLIL::Design &des) {
  makeMapModules(des.modules_);
  const IdDict<RTLIL::Module *> &modules = des.modules_;
  for (const auto &[str, module]: modules) {
    debug << fmt::format("Module:\n name: {} index: {}\n", str.str(), str.index_);
    curModule.indexModule = str.index_;
    walkModule(module);
  }
}

std::string YosysConverterFirrtl::readIdString(const RTLIL::IdString &str) {
  std::string id = "name: ";
  id += str.c_str();
  return id;
}

bool YosysConverterFirrtl::hasIllegalSymbols(const std::string &inputStr) {
  for (char c: inputStr) {
    switch(c) {
    default:
      continue;
    case '$':
    case '\\':
    case '[':
    case '.':
      return true;
    }
  }
  return false;
}

std::string YosysConverterFirrtl::checkName(std::string wireName) {
  wireName.erase(0, 1);
  if (hasIllegalSymbols(wireName)) {
    wireName = getName();
  }
  return wireName;
}

void YosysConverterFirrtl::walkWires(
    const IdDict<RTLIL::Wire *> &ywires) {
  for (const auto &[str, ywire]: ywires) {
    Signal *newSig = new Signal;
    bool portOutput = ywire->port_output;
    bool portInput = ywire->port_input;
    unsigned index = str.index_;
    std::string wireName = str.str();
    newSig->id = checkName(wireName);
    newSig->width = ywire->width;
    std::string modePin;
    if (portInput) {
      modePin = " input";
      newSig->mode = PM_Input;

    }
    if (portOutput) {
      modePin = " output";
      newSig->mode = PM_Output;
    }
    if (!portOutput && !portInput) {
      modePin = " wire";
      newSig->mode = PM_Wire;
    }
    PinMode modePinSig = newSig->mode;
    if (modePinSig != PM_Wire) {
      std::pair<int, std::string> key = { curModule.indexModule, newSig->id };
      portsMode.emplace(key, modePinSig);
      newSig->isUsed = true;
    }
    newSig->type = T_UInt;
    newSig->isDecl = true;
    curModule.signals.emplace(index, newSig);
    fmt::print(debug, "  index: {} {} {} width: {}, name FIRRTL: {}\n",
        index,
        wireName,
        modePinSig,
        std::to_string(ywire->width),
        newSig->id);
  }
}

bool YosysConverterFirrtl::isChunk(
    const RTLIL::SigSpec &first, const RTLIL::SigSpec &second) {
  return first.is_chunk() && second.is_chunk();
}

std::string YosysConverterFirrtl::getConst(const RTLIL::Const &opConst) {
  std::string mean;
  const std::vector<RTLIL::State> &bits = opConst.bits;
  for (const auto &c: bits) {
    mean = getStateString(c) + mean;
  }
  return "0b" + mean;
}

int YosysConverterFirrtl::getIdMemory(const RTLIL::Const &opConst) {
  std::string decode = opConst.decode_string();
  return RTLIL::IdString::get_reference(decode.c_str());
}

std::string YosysConverterFirrtl::getStateString(const RTLIL::State state) {
  switch (state) {
  case RTLIL::S0:
    return "0";
  case RTLIL::S1:
    return "1";
  case RTLIL::Sx: // undefined value or conflict
    return "x";
  case RTLIL::Sz: // high-impedance / not-connected
    return "z";
  case RTLIL::Sa: // don't care (used only in cases)
    return "a";
  case RTLIL::Sm: // marker (used internally by some passes)
    return "m";
  default:
    return "";
  }
}

bool YosysConverterFirrtl::hasDontCareBits(const std::string &value) {
  for (auto it = value.begin() + 2, end = value.end(); it != end; ++it) {
    if (*it == 'x') {
      return true;
    }
  }
  return false;
}

template <typename T>
static void addToEndVec(std::vector<T> &vec1, const std::vector<T> &vec2) {
  vec1.insert(vec1.end(), vec2.begin(), vec2.end());
}

int YosysConverterFirrtl::genDontCareBits(Signal *sig) {
  std::vector<int> catWires;
  std::string &mean = sig->mean;
  int length = mean.size() - 1;
  for (int i = length; i > 1; --i) {
    SigAssign sa;
    int newWire = generateGenWire(1);
    sa.lhs = getSignal(newWire);
    sa.lhs->isUsed = true;
    if (mean[i] == 'x') {
      mean[i] = '0';
      int newWire = generateGenWire(1);
      sa.op1.sig = getSignal(newWire);
      sa.op1.sig->isInvalid = true;
      sa.op1.sig->isUsed = true;
      sa.op1.hi = -1;
    } else if ((mean[i] == '0') ||
               (mean[i] == '1')) {
      sa.op1.sig = sig;
      sa.op1.hi = length - i - 2;
      sa.op1.lo = length - i - 2;
    } else {
      debug << mean << "\n";
      assert(0 && "Unsupported Const");
    }
    sa.func = O_Assign;
    delayedAssigns.push_back(sa);
    catWires.push_back(newWire);
  }
  std::vector<SigAssign> tmpSA = tempAssigns;
  tempAssigns.clear();
  int newWire = makeCatWire(catWires);
  addToEndVec(delayedAssigns, tempAssigns);
  tempAssigns = tmpSA;
  return newWire;
}

int YosysConverterFirrtl::generateConstSig(const RTLIL::SigSpec &sigWire) {
  std::string mean = getConst(sigWire.as_const());
  int curIndex = generateConst(mean);
  Signal *sig = getSignal(curIndex);
  if (hasDontCareBits(sig->mean)) {
    return genDontCareBits(sig);
  }
  return curIndex;
}

int YosysConverterFirrtl::generateConst(const std::string &digit) {
  Signal *sig = new Signal;
  sig->mean = digit;
  sig->isConst = true;
  int curIndex = curModule.newIndex;
  curModule.signals.emplace(curIndex, sig);
  --curModule.newIndex;
  return curIndex;
}

size_t YosysConverterFirrtl::countWidth(int index) {
  Signal *sig = getSignal(index);
  return sig->width;
}

int YosysConverterFirrtl::generateGenWire(int width) {
  auto *sig = new Signal;
  sig->isDecl = true;
  sig->mode = PM_Wire;
  sig->type = T_UInt;
  sig->width = width;
  sig->id = getName();
  int newIndex = curModule.newIndex;
  curModule.signals.emplace(newIndex, sig);
  --curModule.newIndex;
  return newIndex;
}

int YosysConverterFirrtl::makeCat(const RTLIL::SigSpec &sigWire) {
  std::vector<int> vecSig;
  std::vector<std::pair<int, int>> vecSigParms;
  const std::vector<RTLIL::SigChunk> &vec = sigWire.chunks();
  for (const auto &sig: vec) {
    vecSig.push_back(deterSigSpec(sig));
    vecSigParms.push_back(deterSigSpecBits(sig));
  }
  int indWire = generateGenWire(-1);
  curModule.operators.emplace(indWire, O_Cat);
  RhsOperands leafs;
  leafs.indexOperands = {vecSig[0], vecSig[1]};
  leafs.parmsOperands = {vecSigParms[0], vecSigParms[1]};
  curModule.yosysCells.emplace(indWire, leafs);
  for (size_t ind = 2; ind < vecSig.size(); ++ind) {
    int oldWire = indWire;
    indWire = generateGenWire(-1);
    curModule.operators.emplace(indWire, O_Cat);
    RhsOperands leafs;
    leafs.indexOperands = {oldWire, vecSig[ind]};
    leafs.parmsOperands = {{ -1, 0 }, vecSigParms[ind]};
    curModule.yosysCells.emplace(indWire, leafs);
  }
  return indWire;
}

int YosysConverterFirrtl::deterSigSpec(const RTLIL::SigSpec &sigWire) {
  int idWire = 0;
  if (sigWire.is_chunk()) {
    const RTLIL::Wire *sig = sigWire.as_chunk().wire;
    if (sig != nullptr) {
      idWire = sig->name.index_;
    } else {
      idWire = generateConstSig(sigWire);
    }
  } else if (sigWire.chunks().size() > 1) {
    idWire = makeCat(sigWire);
  } else {
    assert(0 && "Not supported SigSpec");
    return -1;
  }
  assert(idWire && "Not registred SigSpec");
  return idWire;
}

std::pair<int, int> YosysConverterFirrtl::deterSigSpecBits(
    const RTLIL::SigSpec &sigWire) {
  std::pair<int, int> pair = { -1, 0 };
  if (sigWire.is_chunk()) {
      const RTLIL::Wire *sig = sigWire.as_chunk().wire;
      if (sig != nullptr) {
        RTLIL::SigChunk signal = sigWire.as_chunk();
        int width = signal.width;
        int offset = signal.offset;
        pair.first = width + offset - 1;
        pair.second = offset;
      }
  }
  return pair;
}

std::string YosysConverterFirrtl::determineSigSpec(
    const RTLIL::SigSpec &sigWire) {
  std::string idWire;
  if (sigWire.is_chunk()) {
    const RTLIL::SigChunk chunk = sigWire.as_chunk();
    if (chunk.is_wire()) {
      const RTLIL::IdString &id = chunk.wire->name;
      idWire = fmt::format("{} index: {} width: {} offset: {} upto: {}",
          readIdString(id),
          id.index_,
          chunk.width,
          chunk.offset,
          chunk.wire->upto);
    } else {
      idWire = "mean: " + getConst(sigWire.as_const());
    }
  } else if (sigWire.is_wire()) {
    const RTLIL::IdString &id = sigWire.as_wire()->name;
    idWire = fmt::format("{} index: {}", readIdString(id), id.index_);
  } else if (sigWire.is_fully_const()) {
    idWire = "fully const";
  } else if (sigWire.is_fully_def()) {
    idWire = "fully def";
  } else if (sigWire.is_fully_undef()) {
    idWire = "fully undef";
  }
  return idWire;
}

bool YosysConverterFirrtl::isMemoryType(int index) {
  return (index == ID($memrd).index_)
         || (index == ID($memrd_v2).index_)
         || (index == ID($memwr).index_)
         || (index == ID($memwr_v2).index_);
}

YosysConverterFirrtl::Statement
YosysConverterFirrtl::determineStatement(int index) {
  if (modules.find(index) != modules.end()) {
    return S_Inst;
  }
  if (isMemoryType(index)) {
    return S_Memory;
  }
  if (index == ID($dff).index_) {
    return S_Dff;
  }
  if (index == ID($adff).index_) {
    return S_Adff;
  }
  return S_Connect;
}

bool YosysConverterFirrtl::determineClkPolarity(
    const IdDict<RTLIL::Const> &parms) {
  for (const auto &[str, mean]: parms) {
    if (str.str() == SID_CLK_POLARITY) {
      return mean.as_bool();
    }
  }
  assert(0 && "Unsupported format parameters of dff cell");
  return false;
}

bool YosysConverterFirrtl::determineRstPolarity(
    const IdDict<RTLIL::Const> &parms) {
  for (const auto &[str, mean]: parms) {
    if (str.str() == SID_ARST_POLARITY) {
      return mean.as_bool();
    }
  }
  assert(0 && "Unsupported format parameters of dff cell");
  return false;
}

int YosysConverterFirrtl::makePolaritySig(bool posedge, int sig) {
  int indexSig = 0;
  if (!posedge) {
    SigAssign sa;
    indexSig = generateGenWire(-1);
    sa.lhs = getSignal(indexSig);
    sa.op1.sig = getSignal(sig);
    sa.func = O_Neg;
    delayedAssigns.push_back(sa);
  }
  return indexSig;
}

int YosysConverterFirrtl::makePolarityDriverSig(bool posedge, int clk) {
  int indexSig = makePolaritySig(posedge, clk);
  SigAssign sa;
  int newClk;
  if (indexSig == 0) {
    newClk = makeDriverSignal(clk, &sa);
  } else {
    newClk = makeDriverSignal(indexSig, &sa);
  }
  delayedAssigns.push_back(sa);
  return newClk;
}

int YosysConverterFirrtl::makePolarityRstSig(bool posedge, const int rst) {
  int indexSig = makePolaritySig(posedge, rst);
  if (indexSig == 0) {
    return rst;
  }
  return indexSig;
}

void replaceElementVector(std::vector<int> &vec, int num1, int num2) {
  for (size_t i = 0; i < vec.size(); ++i) {
    if (vec[i] == num1) {
      vec[i] = num2;
    }
  }
}

void YosysConverterFirrtl::makeRenameOutput(int index) {
  Signal *output = getSignal(index);
  output->mode = PM_Wire;
  int newSig = generateGenWire(output->width);
  Signal *newOutput = getSignal(newSig);
  newOutput->isUsed = true;
  newOutput->mode = PM_Output;
  replaceElementVector(curModule.orderPorts, index, newSig);
  std::string newNameReg = newOutput->id;
  newOutput->id = output->id;
  output->id = newNameReg;
  RhsOperands leafs;
  leafs.indexOperands.push_back(index);
  leafs.parmsOperands.push_back({ -1, 0 });
  curModule.yosysCells.emplace(newSig, leafs);
  curModule.operators.emplace(newSig, O_Assign);
}

int YosysConverterFirrtl::deterDffLHS(const RTLIL::SigSpec &sig) {
  if (sig.chunks().size() > 1) {
    assert(0 && "Not determine mode signal");
  }
  const RTLIL::Wire *sigWire = sig.as_chunk().wire;
  if (sigWire != nullptr) {
    return sigWire->name.index_;
  }
  assert (0 && "LHS is const");
  return 0;
}

YosysConverterFirrtl::FlipFlop YosysConverterFirrtl::fillPortsDFF(
    bool isAsync, const IdDict<RTLIL::SigSpec> &cons) {
  FlipFlop info;
  for (const auto &[str, sig]: cons) {
    std::string strParm = str.str();
    if (strParm == SID_CLK) {
      info.clk = deterSigSpec(sig);
    } else if (strParm == SID_D) {
      info.data = deterSigSpec(sig);
      info.offsetDataPort = deterSigSpecBits(sig);
    } else if (strParm == SID_Q) {
      info.lhs = deterDffLHS(sig);
      if (getSignal(info.lhs)->mode == PM_Output) {
        makeRenameOutput(info.lhs);
      }
      info.out = buildLHS(sig);
    } else if (strParm == SID_ARST && isAsync) {
      info.rst = deterSigSpec(sig);
    } else {
      assert(0 && "Unsupported format dff cells\n");
    }
  }
  return info;
}

void YosysConverterFirrtl::makeDFF(const RTLIL::Cell *cell, bool isAsync) {
  std::string initValue;
  FlipFlop info = fillPortsDFF(isAsync, cell->connections_);
  Signal *output = getSignal(info.out);
  bool posedge = determineClkPolarity(cell->parameters);
  info.clk = makePolarityDriverSig(posedge, info.clk);
  output->driverSig = getSignal(info.clk)->id;
  output->mode = PM_Reg;
  if (isAsync) {
    output->mode = PM_Regreset;
    posedge = determineRstPolarity(cell->parameters);
    info.rst = makePolarityRstSig(posedge, info.rst);
    initValue = determineInitValue(cell->parameters);
    output->resetSig = getSignal(info.rst)->id;
    output->resetMean = initValue;
  }
  RhsOperands leafs;
  leafs.indexOperands.push_back(info.data);
  leafs.parmsOperands.push_back(info.offsetDataPort);
  curModule.yosysCells.emplace(info.out, leafs);
  curModule.operators.emplace(info.out, O_Assign);
}

std::string YosysConverterFirrtl::determineInitValue(
    const IdDict<RTLIL::Const> &parms) {
  auto it = parms.find(SID_ARST_VALUE);
  if (it != parms.end()) {
    return getConst(it->second);
  }
  assert(0 && "Unsupported format parameters of dff cell");
  return "";
}

void YosysConverterFirrtl::makeADFF(const RTLIL::Cell *cell) {
  makeDFF(cell, true);
}

void YosysConverterFirrtl::makeInstance(
    const int typeFunction,
    const IdDict<RTLIL::SigSpec> &cons,
    const std::string &nameInst) {
  Instruction instr;
  instr.statement = S_Inst;
  Instance instance;
  instance.indexNestedModule = typeFunction;
  instance.idInstance = nameInst;
  instance.nameNestedModule = modulesName[typeFunction];
  const IdDict<RTLIL::SigSpec> &connects = cons;
  for (const auto &[str, sig]: connects) {
    Signal *op1, *op2;
    int ind = sig.as_chunk().wire->name.index_;
    op2 = getSignal(ind);
    op1 = new Signal;
    op1->mode = op2->mode;
    op1->width = op2->width;
    op1->type = op2->type;
    op1->id = str.c_str();
    op1->id.erase(0, 1);
    op1->isUsed = true;
    op2->isUsed = true;
    curModule.genSig.push_back(op1);
    DataPorts ports;
    ports.params = deterSigSpecBits(sig);
    ports.signals = { op1, op2 };
    instance.ports.push_back(ports);
  }
  instr.instance = instance;
  curModule.instructions.push_back(instr);
}

std::string YosysConverterFirrtl::binaryToDecimal(const std::string &binStr) {
  size_t decimal = std::stoul(binStr, nullptr, 2);
  return std::to_string(decimal);
}

void YosysConverterFirrtl::checkShiftOperator(
    Operator &operator_, RhsOperands &leafs) {
  if (operator_ == O_Shl || operator_ == O_Shr) {
    std::string &value = getSignal(leafs.indexOperands[0])->mean;
    if (!value.empty()) {
      value = binaryToDecimal(value);
      leafs.parmsOperands[0] = { -1, 0 };
    } else {
      if (operator_ == O_Shl) {
        operator_ = O_Dshl;
      } else {
        operator_ = O_Dshr;
      }
    }
  }
}

template<typename T>
static void reverse(T &vec) {
  std::reverse(vec.begin(), vec.end());
}

void YosysConverterFirrtl::checkPad(
    int firstParms, Operator operator_, RhsOperands &leafs) {
  if (operator_ == O_Pad) {
    int parm = firstParms - (leafs.parmsOperands[0].first - leafs.parmsOperands[0].second + 1);
    leafs.indexOperands.push_back(generateConst(std::to_string(parm)));
    reverse(leafs.indexOperands);
    leafs.parmsOperands.push_back({ -1, 0 });
    reverse(leafs.parmsOperands);
  }
}

void YosysConverterFirrtl::makeUnaryConnect(
    int typeFunction, const IdDict<RTLIL::SigSpec> &cons) {
  int root;
  RhsOperand leafA;
  const IdDict<RTLIL::SigSpec> &connects = cons;
  for (const auto &[str, sig]: connects) {
    std::string strParm = str.str();
    if (strParm == SID_Y) {
      root = buildLHS(sig);
    } else if (strParm == SID_A) {
      leafA.index = deterSigSpec(sig);
      leafA.parm = deterSigSpecBits(sig);
    } else {
      assert(0 && "Unsupported format unary cell");
    }
  }
  RhsOperands leafs;
  std::vector<RhsOperand> leafsInit = { leafA };
  unifyRhsOperands(leafs, leafsInit);
  curModule.operators.emplace(root, logicFunction(typeFunction));
  curModule.yosysCells.emplace(root, leafs);
}

void YosysConverterFirrtl::makeBinaryConnect(
    int typeFunction, const IdDict<RTLIL::SigSpec> &cons) {
  int root;
  Operator operator_ = logicFunction(typeFunction);
  RhsOperand leafA, leafB;
  const IdDict<RTLIL::SigSpec> &connects = cons;
  std::pair<int, int> pairParm;
  int firstParms = 0;
  for (const auto &[str, sig]: connects) {
    std::string strParm = str.str();
    if (strParm == SID_Y) {
      root = buildLHS(sig);
      pairParm = deterSigSpecBits(sig);
      firstParms = pairParm.first - pairParm.second + 1;
    } else if (strParm == SID_A) {
      leafA.index = deterSigSpec(sig);
      leafA.parm = deterSigSpecBits(sig);
    } else if (strParm == SID_B) {
      leafB.index = deterSigSpec(sig);
      leafB.parm = deterSigSpecBits(sig);
    } else {
      assert(0 && "Unsupported format binarny cell");
    }
  }
  RhsOperands leafs;
  std::vector<RhsOperand> leafsInit = {leafB, leafA};
  unifyRhsOperands(leafs, leafsInit);
  checkPad(firstParms, operator_, leafs);
  checkShiftOperator(operator_, leafs);
  curModule.operators.emplace(root, operator_);
  curModule.yosysCells.emplace(root, leafs);
}

void YosysConverterFirrtl::printCell(const RTLIL::Cell *cell) {
  debug << " Connections:\n";
  const IdDict<RTLIL::SigSpec> &cons = cell->connections_;
  std::string connections;
  for (const auto &[str, sig]: cons) {
    std::string parms;
    if (sig.chunks().size() == 1) {
      parms = fmt::format(" width: {} offset: {}",
          sig.as_chunk().width,
          sig.as_chunk().offset);
    }
    connections += fmt::format("   {} index: {} : {}{}\n",
        readIdString(str),
        std::to_string(str.index_),
        Yosys::log_signal(sig),
        parms);
  }
  debug << " Parameters:\n";
  const IdDict<RTLIL::Const> &parameters = cell->parameters;
  for (const auto &[str, constant]: parameters) {
    debug << fmt::format("   {} index: {} : {}\n",
        readIdString(str),
        std::to_string(str.index_),
        getConst(constant));
  }
  debug << "\n";
}

YosysConverterFirrtl::Memory *YosysConverterFirrtl::getMemory(
    const IdDict<RTLIL::Const> &parameters) {
  for (const auto &[str, constant]: parameters) {
    if (str.str() == SID_MEMID) {
      int idMemory = getIdMemory(constant);
      debug << fmt::format("{} memory \n", idMemory);
      return &curModule.memories[idMemory];
    }
  }
  assert(0 && "Unsupported format memory cell\n");
  return nullptr;
}

bool YosysConverterFirrtl::isReadMemory(int index) {
  return (index == ID($memrd).index_)
         || (index == ID($memrd_v2).index_);
}

bool YosysConverterFirrtl::isWriteMemory(int index) {
  return (index == ID($memwr).index_)
         || (index == ID($memwr_v2).index_);
}

void YosysConverterFirrtl::fillMask(
    Memory *mem,
    Instruction &instr,
    Controller &controller) {
  if (controller.type == TC_Writer) {
    Signal *sig = getSignal(generateGenWire(-1));
    sig->isUsed = false;
    sig->isDecl = false;
    sig->id = fmt::format("{}.{}.mask", mem->name, controller.name);
    Signal *rhs = getSignal(generateGenWire(1));
    controller.mask = sig;
    rhs->isInvalid = true;
    rhs->isUsed = true;
    SigAssign sa;
    sa.lhs = sig;
    sa.op1.sig = rhs;
    sa.func = O_Assign;
    instr.connects.push_back(sa);
  }
}

void YosysConverterFirrtl::makeController(
    const RTLIL::Cell *cell, Memory *mem) {
  Controller controller;
  controller.type = determineTypeController(cell->type.index_);
  controller.name = getName();
  Instruction instr;
  instr.statement = S_Connect;
  const IdDict<RTLIL::SigSpec> &cons = cell->connections_;
  for (const auto &[str, sig]: cons) {
    Signal *sigNew = getSignal(generateGenWire(-1));
    sigNew->id = fmt::format("{}.{}", mem->name, controller.name);
    Signal **fieldController;
    SigAssign sa;
    sa.func = O_Assign;
    bool inverse = false;
    std::string nameOperand = str.c_str();
    if (nameOperand == SID_DATA) {
        sigNew->id += ".data";
        inverse = true;
        fieldController = &controller.data;
    } else if (nameOperand == SID_EN) {
        sigNew->id += ".en";
        fieldController = &controller.en;
    } else if (nameOperand == SID_CLK) {
        sigNew->id += ".clk";
        sa.func = O_AsClock;
        fieldController = &controller.clk;
    } else if (nameOperand == SID_ADDR) {
        sigNew->id += ".addr";
        fieldController = &controller.addr;
    } else {
        continue;
    }
    sigNew->isUsed = false;
    sigNew->isDecl = false;
    int rhs = deterSigSpec(sig);
    sa.lhs = sigNew;
    Signal *rhsSig = getSignal(rhs);
    rhsSig->isUsed = true;
    rhsSig->isDecl = true;
    *fieldController = rhsSig;
    sa.op1.sig = rhsSig;
    if (inverse && (controller.type == TC_Reader)) {
      sa.lhs = rhsSig;
      sa.op1.sig = sigNew;
    }
    instr.connects.push_back(sa);
  }
  fillMask(mem, instr, controller);
  curModule.instructions.push_back(instr);
  mem->controllers.push_back(controller);
}

YosysConverterFirrtl::TypeController
YosysConverterFirrtl::determineTypeController(int index) {
  TypeController type = TC_ReadWriter;
  if (isReadMemory(index)) {
    type = TC_Reader;
  } else if (isWriteMemory(index)) {
    type = TC_Writer;
  }
  if (type == TC_ReadWriter) {
    assert(0 && "Unsopprted format memory cell\n");
  }
  return type;
}

void YosysConverterFirrtl::makeMemory(const RTLIL::Cell *cell) {
  Memory *mem = getMemory(cell->parameters);
  makeController(cell, mem);
}

void YosysConverterFirrtl::unifyRhsOperands(
    RhsOperands &leafs, const std::vector<RhsOperand> &leafsVec) {
  for (const auto &leaf: leafsVec) {
    leafs.indexOperands.push_back(leaf.index);
    leafs.parmsOperands.push_back(leaf.parm);
  }
}

void YosysConverterFirrtl::insertData(
    const RTLIL::SigChunk &chunk,
    const int lhsWire,
    const int length) {
  int index = deterSigSpec(chunk);
  std::pair<int, int> parm = deterSigSpecBits(chunk);
  RhsDeps data;
  data.indexRhs = lhsWire;
  data.bitRhsHi = length;
  data.bitRhsLo = length - chunk.width + 1;
  data.bitLhsHi = parm.first;
  data.bitLhsLo = parm.second;
  std::map<int, std::vector<RhsDeps>> &deps = curModule.depsLHS;
  if (deps.find(index) != deps.end()) {
    std::vector<RhsDeps> &dataVec = deps[index];
    dataVec.push_back(data);
  } else {
    std::vector<RhsDeps> dataVec = { data };
    deps.emplace(index, dataVec);
  }
  if (parm.first == -1) {
    assert(0 && "Error to determine parameters of lhs");
  }
}

void YosysConverterFirrtl::printDeps() {
  std::string lhsPrint;
  for (auto [index, vec]: curModule.depsLHS) {
    lhsPrint += fmt::format("LHS: {} Data: \n", getSignal(index)->id);
    for (auto it: vec) {
      lhsPrint += fmt::format(" LhsBitLo {}\n LhsBitHi {}\n RhsBitLo {}\n RhsBitHi {}\n  RhsIndex {}\n",
          it.bitLhsLo,
          it.bitLhsHi,
          it.bitRhsLo,
          it.bitRhsHi,
          getSignal(it.indexRhs)->id);
    }
  }
  debug << fmt::format(":::::Deps:\n{}:::endDeps\n", lhsPrint);
}

int YosysConverterFirrtl::deterLengthLhs(const RTLIL::SigSpec &lhs) {
  int length = 0;
  const std::vector<RTLIL::SigChunk> &vec = lhs;
  for (const auto &chunk: vec) {
    length += chunk.width;
  }
  return length;
}

int YosysConverterFirrtl::buildLHS(const RTLIL::SigSpec &lhs) {
  std::pair<int, int> parm = deterSigSpecBits(lhs);
  int width = parm.first - parm.second + 1;
  int lhsWire = generateGenWire(width);

  const std::vector<RTLIL::SigChunk> &chunks = lhs.chunks();
  assert(!chunks.empty() && "Problem to determine LHS");

  int offset = deterLengthLhs(lhs) - 1;
  for (const auto &chunk: chunks) {
    insertData(chunk, lhsWire, offset);
    offset -= chunk.width;
    assert(offset >= -1 && "Problem with determine bytes of rhs expr");
  }
  return lhsWire;
}

void YosysConverterFirrtl::makeMux(const IdDict<RTLIL::SigSpec> &cons) {
  int root;
  RhsOperand leafS, leafA, leafB;
  const IdDict<RTLIL::SigSpec> &connects = cons;
  for (const auto &[str, sig]: connects) {
    int index = 0;
    std::pair<int, int> parm = { -1, 0 };
    std::string strParm = str.str();
    if (strParm != SID_Y) {
      index = deterSigSpec(sig);
      parm = deterSigSpecBits(sig);
    }
    if (strParm == SID_Y) {
      root = buildLHS(sig);
    } else if (strParm == SID_S) {
      leafS.index = index;
      leafS.parm = parm;
    } else if (strParm == SID_A) {
      getSignal(index)->isInvalid = true;
      leafA.index = index;
      leafA.parm = parm;
    } else if (strParm == SID_B) {
      getSignal(index)->isInvalid = true;
      leafB.index = index;
      leafB.parm = parm;
    } else {
      debug << strParm;
      assert(0 && "Unsupported format mux cells");
    }
  }
  RhsOperands leafs;
  std::vector<RhsOperand> leafVec = { leafS, leafB, leafA };
  unifyRhsOperands(leafs, leafVec);
  curModule.operators.emplace(root, O_Mux);
  curModule.yosysCells.emplace(root, leafs);
}

bool YosysConverterFirrtl::isMux(int index) {
  return (index == ID($mux).index_) || (index == ID($ternary).index_);
}

void YosysConverterFirrtl::walkCells(const IdDict<RTLIL::Cell *> &ycells) {
  for (const auto &[str, cell]: ycells) {
    std::string nameCell = str.c_str();
    nameCell.erase(0, 1);
    debug << "=================================================\n";
    int typeFunction = cell->type.index_;
    Statement statement = determineStatement(typeFunction);
    if (statement == S_Connect) {
      if (isMux(typeFunction)) {
        makeMux(cell->connections_);
      } else {
        Operator operator_ = logicFunction(typeFunction);
        int arity = determineTypeOperator(operator_);
        if (arity == 1) {
          makeUnaryConnect(typeFunction, cell->connections_);
        } else if (arity == 2) {
          makeBinaryConnect(typeFunction, cell->connections_);
        } else {
          assert(0 && "Unsupported cell");
        }
      }
    } else if (statement == S_Inst) {
      makeInstance(typeFunction, cell->connections_, nameCell);
    } else if (statement == S_Memory) {
      makeMemory(cell);
    } else if (statement == S_Dff) {
      makeDFF(cell);
    } else if (statement == S_Adff) {
      makeADFF(cell);
    }
    debug << fmt::format(" Cell: {} index: {}\nType of cell: {}\n",
        nameCell,
        str.index_,
        typeFunction);
    printCell(cell);
  }
}

bool YosysConverterFirrtl::isUnOperator(Operator func) {
  switch (func) {
  case O_Not:
  case O_Orr:
  case O_Xorr:
  case O_Andr:
  case O_Neg:
  case O_Bits:
  case O_AsClock:
  case O_AsAsyncReset:
    return true;
  default:
    return false;
  }
}

bool YosysConverterFirrtl::isBinOperator(Operator func) {
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
  case O_Xnor:
  case O_Nor:
  case O_Nand:
    return true;
  default:
    return false;
  }
}

bool YosysConverterFirrtl::isTernOperator(Operator func) {
  if (func == O_Mux) {
    return true;
  }
  return false;
}

int YosysConverterFirrtl::determineTypeOperator(Operator func) {
  if (isUnOperator(func)) {
    return 1;
  }
  if (isBinOperator(func)) {
    return 2;
  }
  if (isTernOperator(func)) {
    return 3;
  }
  return 0;
}

void YosysConverterFirrtl::requireOperand(
    Operand &op,
    int index,
    int hi,
    int lo) {
  op.sig = getSignal(index);
  op.sig->isUsed = true;
  op.hi = hi;
  op.lo = lo;
}

void YosysConverterFirrtl::buildAssigns(int root, bool isInvalid) {
  if (curModule.yosysCells.find(root) != curModule.yosysCells.end()) {
    Operator func = curModule.operators[root];
    int arity = determineTypeOperator(func);
    SigAssign sa;
    sa.func = func;
    Signal *lhs = getSignal(root);
    if (isInvalid) {
      determineInvalid(lhs);
    }
    sa.lhs = lhs;
    sa.lhs->isUsed = true;
    RhsOperands &leafs = curModule.yosysCells[root];
    buildAssigns(leafs.indexOperands[0], isInvalid);
    requireOperand(
        sa.op1,
        leafs.indexOperands[0],
        leafs.parmsOperands[0].first,
        leafs.parmsOperands[0].second);
    if (arity == 2) {
      buildAssigns(leafs.indexOperands[1], isInvalid);
      requireOperand(
          sa.op2,
          leafs.indexOperands[1],
          leafs.parmsOperands[1].first,
          leafs.parmsOperands[1].second);
    }
    if (arity == 3) {
      buildAssigns(leafs.indexOperands[1], isInvalid);
      requireOperand(
          sa.op2,
          leafs.indexOperands[1],
          leafs.parmsOperands[1].first,
          leafs.parmsOperands[1].second);
      buildAssigns(leafs.indexOperands[2], isInvalid);
      requireOperand(
          sa.op3,
          leafs.indexOperands[2],
          leafs.parmsOperands[2].first,
          leafs.parmsOperands[2].second);
    }
    tempAssigns.push_back(sa);
  }
}

YosysConverterFirrtl::Signal *YosysConverterFirrtl::getSignal(int index) {
  Signal *sig;
  if (curModule.signals.find(index) != curModule.signals.end()) {
    sig = curModule.signals[index];
  } else {
    assert(0 && "Not registred signal");
  }
  return sig;
}

void YosysConverterFirrtl::printAllYosysCells() {
  const std::map<int, RhsOperands> &ycell = curModule.yosysCells;
  for (const auto &[root, leaf]: ycell) {
    int arity = leaf.indexOperands.size();
    Signal *lhs = getSignal(root);
    RhsOperands &leafs = curModule.yosysCells[root];
    std::string idOp[3];
    for (int i = 0; i < arity; ++i) {
      idOp[i] = getSignal(leafs.indexOperands[0])->id;
    }
    fmt::print(debug, "Yosys cells:\n  Lhs: {} RHS: {} {} {}",
        lhs->id,
        idOp[0],
        idOp[1],
        idOp[2]);
  }
}

void YosysConverterFirrtl::walkAllYosysCells() {
  const std::map<int, RhsOperands> &ycell = curModule.yosysCells;
  debug << "Yosys cells:\n";
  for (const auto &[root, leaf]: ycell) {
    Operator func = curModule.operators[root];
    int arity = determineTypeOperator(func);
    SigAssign sa;
    sa.func = func;
    Signal *lhs = getSignal(root);
    sa.lhs = lhs;
    sa.lhs->isUsed = true;
    RhsOperands &leafs = curModule.yosysCells[root];
    requireOperand(
        sa.op1,
        leafs.indexOperands[0],
        leafs.parmsOperands[0].first,
        leafs.parmsOperands[0].second);
    if (arity == 2) {
      requireOperand(
          sa.op2,
          leafs.indexOperands[1],
          leafs.parmsOperands[1].first,
          leafs.parmsOperands[1].second);
    }
    if (arity == 3) {
      requireOperand(
          sa.op2,
          leafs.indexOperands[1],
          leafs.parmsOperands[1].first,
          leafs.parmsOperands[1].second);
      requireOperand(
          sa.op3,
          leafs.indexOperands[2],
          leafs.parmsOperands[2].first,
          leafs.parmsOperands[2].second);
    }
    tempAssigns.push_back(sa);
  }
}

bool YosysConverterFirrtl::compareByBitLhsLo(
    const RhsDeps &lhs1, const RhsDeps &lhs2) {
  if (lhs1.bitLhsLo == lhs2.bitLhsLo) {
    assert(0 && "Intersection in the bits");
  }
  return lhs1.bitLhsLo < lhs2.bitLhsLo;
}

void YosysConverterFirrtl::makeCatRhs(
    int indexLhs, const std::vector<RhsDeps> &vec) {
  const int lastIndex = static_cast<int>(vec.size()) - 1;

  RhsOperand leaf2;
  leaf2.index = vec[lastIndex].indexRhs;
  leaf2.parm = { vec[lastIndex].bitRhsHi, vec[lastIndex].bitRhsLo };

  for (int i = lastIndex - 1; i >= 0; --i) {
    RhsOperand leaf1;
    leaf1.index = vec[i].indexRhs;
    leaf1.parm = { vec[i].bitRhsHi, vec[i].bitRhsLo };

    RhsOperands leafs;
    std::vector<RhsOperand> leafsVec = { leaf1, leaf2 };
    unifyRhsOperands(leafs, leafsVec);

    int wireId = generateGenWire(-1);
    curModule.operators.emplace(wireId, O_Cat);
    curModule.yosysCells.emplace(wireId, std::move(leafs));

    leaf2.index = wireId;
    leaf2.parm = { -1, 0 };
  }

  RhsOperands leafs;
  std::vector<RhsOperand> leafsVec = { leaf2 };
  unifyRhsOperands(leafs, leafsVec);
  curModule.operators.emplace(indexLhs, O_Assign);
  curModule.yosysCells.emplace(indexLhs, std::move(leafs));
}

void YosysConverterFirrtl::walkDepsLhs() {
  std::map<int, std::vector<RhsDeps>> &deps = curModule.depsLHS;
  for (auto &[index, vec]: deps) {
    std::sort(vec.begin(), vec.end(), compareByBitLhsLo);
    makeCatRhs(index, vec);
  }
}

int YosysConverterFirrtl::makeCatSigSpec(const RTLIL::SigSpec &sigWire) {
  std::vector<int> vecSig;
  std::vector<std::pair<int, int>> vecSigParms;
  const std::vector<RTLIL::SigChunk> &vec = sigWire.chunks();
  for (const auto &sig: vec) {
    vecSig.push_back(deterSigSpecRHS(sig));
    vecSigParms.push_back(deterSigSpecBitsRHS(sig));
  }
  const int lastIndex = static_cast<int>(vecSig.size()) - 1;
  RhsOperand leaf2;
  leaf2.index = vecSig[lastIndex];
  leaf2.parm = vecSigParms[lastIndex];

  for (int i = lastIndex - 1; i >= 0; --i) {
    RhsOperand leaf1;
    leaf1.index = vecSig[i];
    leaf1.parm = vecSigParms[i];
    RhsOperands leafs;
    std::vector<RhsOperand> leafsVec = { leaf1, leaf2 };
    unifyRhsOperands(leafs, leafsVec);

    int wireId = generateGenWire(-1);
    curModule.operators.emplace(wireId, O_Cat);
    curModule.yosysCells.emplace(wireId, std::move(leafs));

    leaf2.index = wireId;
    leaf2.parm = { -1, 0 };
  }
  RhsOperands leafs;
  std::vector<RhsOperand> leafsVec = { leaf2 };
  unifyRhsOperands(leafs, leafsVec);
  int indWire = generateGenWire(-1);
  curModule.operators.emplace(indWire, O_Assign);
  curModule.yosysCells.emplace(indWire, std::move(leafs));
  return indWire;
}

int YosysConverterFirrtl::findDataByBitLhsHi(
    const std::vector<RhsDeps> &vec, int searchValue) {
  int result = 0;
  for (const auto &data : vec) {
    if (data.bitLhsHi == searchValue) {
      result = data.indexRhs;
    }
  }
  return result;
}

int YosysConverterFirrtl::deterSigSpecRHS(const RTLIL::SigSpec &sigWire) {
  int idWire = 0;
  if (sigWire.is_chunk()) {
    const RTLIL::Wire *sig = sigWire.as_chunk().wire;
    if (sig != nullptr) {
      idWire = sig->name.index_;
      std::map<int, std::vector<RhsDeps>> &deps = curModule.depsLHS;
      if (deps.find(idWire) != deps.end()) {
        std::pair<int, int> parms = deterSigSpecBits(sigWire);
        int data = findDataByBitLhsHi(curModule.depsLHS[idWire], parms.first);
        if (data != 0) {
          idWire = data;
        }
      }
    } else {
      idWire = generateConstSig(sigWire);
    }
  } else if (sigWire.chunks().size() > 1) {
    idWire = makeCatSigSpec(sigWire);
  } else {
    assert(0 && "Not supported SigSpec");
    return -1;
  }
  assert(idWire && "Not registred SigSpec");
  return idWire;
}

std::pair<int, int> YosysConverterFirrtl::deterSigSpecBitsRHS(
    const RTLIL::SigSpec &sigWire) {
  std::pair<int, int> pair = { -1, 0 };
  if (sigWire.is_chunk()) {
    const RTLIL::Wire *sig = sigWire.as_chunk().wire;
    if (sig != nullptr) {
      std::map<int, std::vector<RhsDeps>> &deps = curModule.depsLHS;
      if (deps.find(sig->name.index_) == deps.end()) {
        RTLIL::SigChunk signal = sigWire.as_chunk();
        int width = signal.width;
        int offset = signal.offset;
        pair.first = width + offset - 1;
        pair.second = offset;
      }
    }
  }
  return pair;
}

void YosysConverterFirrtl::walkConnections(
    const std::vector<std::pair<RTLIL::SigSpec, RTLIL::SigSpec>> &connects) {
  for (const auto &[op1, op2]: connects) {
    int lhs = buildLHS(op1);
    int op = deterSigSpecRHS(op2);
    RhsOperands leafs;
    leafs.indexOperands.push_back(op);
    leafs.parmsOperands.push_back(deterSigSpecBitsRHS(op2));
    curModule.yosysCells.emplace(lhs, leafs);
    curModule.operators.emplace(lhs, O_Assign);
    printConnections(op1, op2);
  }
  Instruction instr;
  instr.statement = S_Connect;
  walkDepsLhs();
  walkAllYosysCells();
  instr.connects = tempAssigns;
  curModule.instructions.push_back(instr);
  tempAssigns.clear();
}

void YosysConverterFirrtl::printConnections(
    const RTLIL::SigSpec &op1, const RTLIL::SigSpec &op2) {
  debug << " Connect:\n";
  debug << fmt::format("  1st operand {} size: {}\n  2st operand {} size: {}\n",
      Yosys::log_signal(op1),
      op1.chunks().size(),
      Yosys::log_signal(op2),
      op2.chunks().size());
}

void YosysConverterFirrtl::walkParameteres(
    const Yosys::hashlib::idict<RTLIL::IdString> &availParms) {
  for (const auto &parameter: availParms) {
    debug << fmt::format(" index: {}  name: {}\n",
        parameter.index_,
        parameter.c_str());
  }
}

void YosysConverterFirrtl::walkPorts(const std::vector<RTLIL::IdString> &ports) {
  for (const auto &port: ports) {
    int index = port.index_;
    debug << fmt::format(" {} index: {}\n", readIdString(port), index);
    curModule.orderPorts.push_back(index);
  }
}

bool YosysConverterFirrtl::isSigSpec(int op1, int op2) {
  return op1 && op2;
}

bool YosysConverterFirrtl::isUndef(
    const RTLIL::SigSpec &sig1, const RTLIL::SigSpec &sig2) {
  return sig1.is_fully_undef() && sig2.is_fully_undef();
}

void YosysConverterFirrtl::determineInvalid(Signal *sig) {
  if (sig->mode == PM_Wire) {
    sig->isInvalid = true;
  }
}

bool YosysConverterFirrtl::containsIndex(
    const std::vector<int>& vec, int index) {
  auto it = std::find(vec.begin(), vec.end(), index);
  return it != vec.end();
}

void YosysConverterFirrtl::fillLhsProc(
    const int lhs,
    const int rhs,
    const std::pair<int, int> parmsLhs,
    int &curBit,
    std::map<int, std::vector<RhsDeps>> &lhsProc) {
  int bitLhsHi = parmsLhs.first;
  int bitLhsLo = parmsLhs.second;
  if (bitLhsHi == -1) {
    assert(0 && "Don't determine lhs bits");
  }
  RhsDeps data;
  data.indexRhs = rhs;
  for (int bit = bitLhsLo; bit <= bitLhsHi; ++bit) {
    data.bitLhs = bit;
    data.bitRhs = curBit;
    lhsProc[lhs].push_back(data);
    ++curBit;
  }
}

bool YosysConverterFirrtl::compareDataByBitLhs(
    const RhsDeps &a, const RhsDeps &b) {
  return a.bitLhs < b.bitLhs;
}

void YosysConverterFirrtl::removeElementsByIndices(
    std::vector<RhsDeps> &targetVector,
    const std::vector<int> &indicesToRemove) {
  std::vector<RhsDeps> newVector;
  for (size_t i = 0; i < targetVector.size(); ++i) {
    if (!containsIndex(indicesToRemove, i)) {
      newVector.push_back(targetVector[i]);
    }
  }
  targetVector = newVector;
}

void YosysConverterFirrtl::keepLastElementsByBitLhs(
    std::vector<RhsDeps> &dataVector) {
  std::vector<int> existsLhsBits;
  std::vector<int> deletedElements;
  for (int i = dataVector.size() - 1; i >= 0; --i) {
    int bit = dataVector[i].bitLhs;
    if (containsIndex(existsLhsBits, bit)) {
      deletedElements.push_back(i);
    } else {
      existsLhsBits.push_back(bit);
    }
  }
  removeElementsByIndices(dataVector, deletedElements);
  std::sort(dataVector.begin(), dataVector.end(), compareDataByBitLhs);
}

void YosysConverterFirrtl::makeAssign(
    int lhs, const std::vector<RhsDeps> &vec) {
  SigAssign sa;
  sa.func = O_Assign;
  int curBit = 0;
  std::vector<int> vecNewWire;
  for (size_t i = 0; i < vec.size();) {
    const RhsDeps &data = vec[i];
    int newWire = generateGenWire(1);
    Signal *sig = getSignal(newWire);
    sig->isUsed = true;
    sig->isInvalid = true;
    if (data.bitLhs == curBit) {
      sa.lhs = sig;
      sa.op1.sig = getSignal(data.indexRhs);
      sa.op1.hi = data.bitRhs;
      sa.op1.lo = data.bitRhs;
      tempAssigns.push_back(sa);
      ++i;
    }
    vecNewWire.push_back(newWire);
    ++curBit;
  }
  int rhs = makeCatWire(vecNewWire, true);
  sa.lhs = getSignal(lhs);
  sa.lhs->isUsed = true;
  sa.lhs->isInvalid = true;
  sa.func = O_Cat;
  int newInvWire = generateGenWire(sa.lhs->width - curBit);
  Signal *newInvSig = getSignal(newInvWire);
  newInvSig->isInvalid = true;
  newInvSig->isUsed = true;
  sa.op2.sig = newInvSig;
  buildAssigns(rhs, true);
  sa.op1.sig = getSignal(rhs);
  sa.op2.hi = -1;
  sa.op1.hi = -1;
  tempAssigns.push_back(sa);
}

void YosysConverterFirrtl::makeLhs(
    const RTLIL::SigSpec &sig,
    const RTLIL::SigSpec &sigRhs,
    std::map<int, std::vector<RhsDeps>> &lhsProc) {
  int curBit = 0;
  const std::vector<RTLIL::SigChunk> chunks = sig.chunks();
  for (size_t i = 0; i < chunks.size(); ++i) {
    const RTLIL::SigSpec &signal = chunks[i];
    int lhs = deterSigSpec(signal);
    int rhs = deterSigSpec(sigRhs);
    buildAssigns(rhs, true);
    std::pair<int, int> parmsLhs = deterSigSpecBits(signal);
    fillLhsProc(lhs, rhs, parmsLhs, curBit, lhsProc);
    keepLastElementsByBitLhs(lhsProc[lhs]);
    makeAssign(lhs, lhsProc[lhs]);
  }
}

void YosysConverterFirrtl::declareOperand(int op, bool SA) {
  Signal *sig = getSignal(op);
  if (!sig->isConst) {
    sig->isUsed = true;
    sig->isDecl = true;
    if (SA) {
      buildAssigns(op);
    }
  }
}

void YosysConverterFirrtl::walkActions(
    const std::vector<RTLIL::SigSig> &actions, bool isInvalid) {
  for (const auto &[sig1, sig2]: actions) {
    debug << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
    printConnections(sig1, sig2);
    int op2 = 0;
    if (!isUndef(sig1, sig2)) {
      int op1 = deterSigSpec(sig1);
      op2 = deterSigSpec(sig2);
      if (containsIndex(tmpBlockedRHS, op2)) {
        if (getSignal(op2)->mode == PM_Output) {
          debug << "***Blocked assign***\n";
          continue;
        }
      }
      debug << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
      makeLhs(sig1, sig2, stackLHS.lhs);
      stackLHS.pushBack(stackLHS.lhs);
      declareOperand(op1);
      declareOperand(op2, true);
    }
  }
}

std::vector<int> YosysConverterFirrtl::findDontCareBits(std::string &value) {
  std::vector<int> indices;
  for (int i = value.length() - 1; i >= 0; --i) {
    if (value[i] == '4') {
      value[i] = '1';
      indices.push_back(value.length() - i);
    }
  }
  return indices;
}

int YosysConverterFirrtl::makeCatWire(std::vector<int> &vec, bool isInvalid) {
  SigAssign sa;
  int newWire = generateGenWire(-1);
  sa.lhs = getSignal(newWire);
  sa.lhs->isUsed = true;
  sa.lhs->isInvalid = isInvalid;
  sa.op1.sig = getSignal(vec[0]);
  sa.op1.sig->isUsed = true;
  sa.op1.sig->isDecl = true;
  sa.func = O_Assign;
  tempAssigns.push_back(sa);
  sa.func = O_Cat;
  for (size_t i = 1; i < vec.size(); ++i) {
    sa.op1.sig = getSignal(newWire);
    sa.op2.sig = getSignal(vec[i]);
    sa.op2.sig->isUsed = true;
    sa.op2.sig->isDecl = true;
    newWire = generateGenWire(-1);
    sa.lhs = getSignal(newWire);
    sa.lhs->isUsed = true;
    sa.lhs->isInvalid = true;
    tempAssigns.push_back(sa);
  }
  return newWire;
}

int YosysConverterFirrtl::assignBit(int op, int bit) {
  SigAssign sa;
  sa.func = O_Assign;
  int newWire = generateGenWire(-1);
  sa.lhs = getSignal(newWire);
  sa.lhs->isUsed = true;
  sa.op1.sig = getSignal(op);
  sa.op1.sig->isUsed = true;
  sa.op1.hi = bit;
  sa.op1.lo = bit;
  tempAssigns.push_back(sa);
  return newWire;
}

std::pair<int, int> YosysConverterFirrtl::makeCat(
    int op2Init, int op1Init, std::vector<int> &indices) {
  std::vector<int> wiresCatOp1;
  std::vector<int> wiresCatOp2;
  int i = 0;
  for (int bit = 0; bit < getSignal(op1Init)->width; bit++) {
    if (indices[i] != bit) {
      wiresCatOp1.push_back(assignBit(op1Init, bit));
      wiresCatOp2.push_back(assignBit(op2Init, bit));
    } else {
      ++i;
    }
  }
  int op1 = makeCatWire(wiresCatOp1);
  int op2 = makeCatWire(wiresCatOp2);
  std::pair<int, int> operands = { op1, op2 };
  return operands;
}

std::pair<int, int> YosysConverterFirrtl::giveDontCareBits(
    const int op2Init, const int op1Init) {
  Signal *sig = getSignal(op2Init);
  if (sig->isConst) {
    std::vector<int> indices = findDontCareBits(sig->mean);
    if (!indices.empty()) {
      std::pair<int, int> operands = makeCat(op2Init, op1Init, indices);
      addToEndVec(tempCases, tempAssigns);
      tempAssigns.clear();
      return operands;
    }
  }
  return { op1Init, op2Init };
}

int YosysConverterFirrtl::makeCondSignal(const RTLIL::SigSpec &signal) {
  int condition = deterSigSpec(signal);
  buildAssigns(condition);
  addToEndVec(tempCases, tempAssigns);
  tempAssigns.clear();
  return condition;
}

void YosysConverterFirrtl::fillParameters(
    const RTLIL::SigSpec &op, int &hi, int &lo) {
  std::pair<int, int> parms = deterSigSpecBits(op);
  hi = parms.first;
  lo = parms.second;
}

int YosysConverterFirrtl::makeCase(
    const RTLIL::SigSpec &op1, const RTLIL::SigSpec &op2) {
  int op1Init = makeCondSignal(op1);
  int op2Init = makeCondSignal(op2);
  std::pair<int, int> operands = giveDontCareBits(op2Init, op1Init);
  int newOp1 = operands.first;
  int newOp2 = operands.second;
  int lhs = generateGenWire(1);
  SigAssign sa;
  sa.lhs = getSignal(lhs);
  sa.lhs->isUsed = true;
  sa.op1.sig = getSignal(newOp1);
  sa.op1.sig->isUsed = true;
  if (newOp1 == op1Init) {
    fillParameters(op1, sa.op1.hi, sa.op1.lo);
  }
  sa.op2.sig = getSignal(newOp2);
  sa.op2.sig->isUsed = true;
  if (newOp2 == op2Init) {
    fillParameters(op2, sa.op2.hi, sa.op2.lo);
  }
  sa.func = O_Eq;
  tempCases.push_back(sa);
  return lhs;
}

void YosysConverterFirrtl::requireCases() {
  Instruction instr;
  instr.statement = S_Connect;
  instr.connects = tempCases;
  tempCases.clear();
  std::vector<Instruction> &ins = curModule.instructions;
  ins.insert(ins.begin(), instr);
}

int YosysConverterFirrtl::makeElse(std::vector<int> &switchers) {
  SigAssign sa;
  int lhs = generateGenWire(-1);
  sa.lhs = getSignal(lhs);
  sa.lhs->isUsed = true;
  sa.op1.sig = getSignal(switchers[0]);
  sa.func = O_Assign;
  tempCases.push_back(sa);
  sa.func = O_Or;
  for (size_t i = 1; i < switchers.size(); ++i) {
    int newLhs = generateGenWire(1);
    sa.lhs = getSignal(newLhs);
    sa.lhs->isUsed = true;
    sa.op1.sig = getSignal(switchers[i]);
    sa.op2.sig = getSignal(lhs);
    tempCases.push_back(sa);
    lhs = newLhs;
  }
  int newLhs = generateGenWire(1);
  sa.lhs = getSignal(newLhs);
  sa.lhs->isUsed = true;
  sa.op1.sig = getSignal(lhs);
  sa.func = O_Not;
  tempCases.push_back(sa);
  return lhs;
}

void YosysConverterFirrtl::walkSwitches(
    const std::vector<RTLIL::SwitchRule *> &switches) {
  Instruction instr;
  instr.statement = S_When;
  walkSwitch(switches, instr);
  curModule.instructions.push_back(instr);
}

void YosysConverterFirrtl::walkSimpleCase(
    const RTLIL::SigSpec &switch_,
    const RTLIL::CaseRule *case_,
    std::vector<int> &switchers,
    Instruction &instr) {
  RTLIL::SigSpec op2;
  const std::vector<RTLIL::SigSpec> &compare = case_->compare;
  debug << " case:" << "\n";
  for (const auto &it: compare) {
    debug << "      " << Yosys::log_signal(it) << "\n";
    op2 = it;
  }
  CondKeyWord keyWord = CKW_If;
  if (compare.empty()) {
    debug << "    else\n";
    int elseWire = makeElse(switchers);
    tempCondStatement.sig = getSignal(elseWire);
  } else {
    int newCase = makeCase(switch_, op2);
    tempCondStatement.sig = getSignal(newCase);
    switchers.push_back(newCase);
  }
  tempCondStatement.branch.push_back(keyWord);
  walkActions(case_->actions, true);
  tempCondStatement.connects = tempAssigns;
  tempAssigns.clear();
  instr.branches.push_back(tempCondStatement);
  walkSwitch(case_->switches, instr);
  tempCondStatement.branch.pop_back();
}

void YosysConverterFirrtl::walkSwitch(
    const std::vector<RTLIL::SwitchRule *> &switches, Instruction &instr) {
  std::vector<int> switchers;
  for (const auto &switcher: switches) {
    std::string typeSignal = determineSigSpec(switcher->signal);
    fmt::print(debug, "   Signal: {} {}\n",
        typeSignal,
        Yosys::log_signal(switcher->signal));

    std::vector<RTLIL::CaseRule *> &cases = switcher->cases;
    for (const auto &case_: cases) {
      if (typeSignal != "fully def") {
        walkSimpleCase(
          switcher->signal, case_, switchers, instr);
      } else {
        walkActions(case_->actions, true);
        if (instr.branches.empty()) {
          addToEndVec(tempCases, tempAssigns);
        } else {
          addToEndVec(instr.branches.back().connects, tempAssigns);
        }
        tempAssigns.clear();
        break;
      }
      stackLHS.popBack();
    }
  }
  requireCases();
}

void YosysConverterFirrtl::walkCaseRule(const RTLIL::CaseRule &caseRule) {
  debug << "  Compare:\n";
  const std::vector<RTLIL::SigSpec> &compare = caseRule.compare;
  for (const auto &sig: compare) {
    debug << "   " << determineSigSpec(sig) << "\n";
  }
  Instruction instr;
  instr.statement = S_Connect;
  debug << "  Actions:\n";
  walkActions(caseRule.actions, true);
  instr.connects = tempAssigns;
  tempAssigns.clear();
  curModule.instructions.push_back(instr);
  debug << "  Switches:\n";
  debug << "______________________\n";
  const std::vector<RTLIL::SwitchRule *> &switches = caseRule.switches;
  walkSwitches(switches);
  debug << "______________________\n";
}

std::string YosysConverterFirrtl::determineSyncType(
    RTLIL::SyncType sync_type) {
  switch (sync_type) {
  case RTLIL::SyncType::ST0:
    return "level0";
  case RTLIL::SyncType::ST1:
    return "level1";
  case RTLIL::SyncType::STp:
    return "posedge";
  case RTLIL::SyncType::STn:
    return "negedge";
  case RTLIL::SyncType::STa:
    return "always active";
  case RTLIL::SyncType::STe:
    return "edge sensitive: both edges";
  case RTLIL::SyncType::STi:
    return "init";
  default:
    return "";
  }
}

int YosysConverterFirrtl::makeDriverSignal(int driverSig, SigAssign *saInit) {
  int newSig = generateGenWire(-1);
  getSignal(newSig)->type = T_Clock;
  SigAssign sa;
  sa.lhs = getSignal(newSig);
  sa.lhs->isUsed = true;
  sa.func = O_AsClock;
  sa.op1.sig = getSignal(driverSig);
  sa.op1.sig->isUsed = true;
  if (saInit == nullptr) {
    tempAssigns.push_back(sa);
  } else {
    *saInit = sa;
  }
  return newSig;
}

void YosysConverterFirrtl::addDelayedAssign(Signal *lhs, Signal *op1) {
  SigAssign sa;
  sa.lhs = lhs;
  sa.lhs->isUsed = true;
  sa.lhs->isDecl = true;
  sa.op1.sig = op1;
  sa.op1.sig->isUsed = true;
  sa.func = O_Assign;
  delayedAssigns.push_back(sa);
}

int YosysConverterFirrtl::copySignal(Signal *sig) {
  Signal *newSig = new Signal;
  newSig->id = sig->id;
  sig->id = getName();
  newSig->type = sig->type;
  newSig->mode = sig->mode;
  newSig->width = sig->width;
  newSig->isUsed = true;
  sig->isUsed = true;
  addDelayedAssign(newSig, sig);
  int newIndex = curModule.newIndex;
  curModule.signals.emplace(newIndex, newSig);
  --curModule.newIndex;
  return newIndex;
}

void YosysConverterFirrtl::redefPorts(
    const std::vector<RTLIL::SigSig> &ports, int driver) {
  const std::vector<RTLIL::SigSig> &portsVec = ports;
  for (const auto &[op1, op2]: portsVec) {
    int oldIndex = deterSigSpec(op1);
    Signal *sig1 = getSignal(oldIndex);
    if (sig1->mode != PM_Reg && sig1->mode != PM_Regreset) {
      int newIndex = copySignal(sig1);
      if (hasInOutMode(sig1)) {
        replaceElementVector(curModule.orderPorts, oldIndex, newIndex);
      }
      sig1->mode = PM_Reg;
    }
    sig1->driverSig = getSignal(driver)->id;
  }
}

int YosysConverterFirrtl::makeMyInit(const RTLIL::SigSpec &op2) {
  SigAssign sa;
  int myInit = generateGenWire(-1);
  sa.lhs = getSignal(myInit);
  sa.lhs->isUsed = true;
  sa.op1.sig = getSignal(deterSigSpec(op2));
  sa.op1.sig->isUsed = true;
  sa.func = O_Assign;
  tempAssigns.push_back(sa);
  return myInit;
}

void YosysConverterFirrtl::redefRstPorts(
    const std::vector<RTLIL::SigSig> &ports, int driver) {
  const std::vector<RTLIL::SigSig> &portsVec = ports;
  for (const auto &[op1, op2]: portsVec) {
    int oldIndex = deterSigSpec(op1);
    Signal *sig1 = getSignal(oldIndex);
    if (sig1->mode != PM_Reg) {
      int newIndex = copySignal(sig1);
      if (hasInOutMode(sig1)) {
        replaceElementVector(curModule.orderPorts, oldIndex, newIndex);
      }
      sig1->mode = PM_Regreset;
    }
    sig1->resetSig = getSignal(driver)->id;
    int myInit = makeMyInit(op2);
    sig1->resetMean = getSignal(myInit)->id;
  }
}

void YosysConverterFirrtl::makeClockSignal(
    int driverSig,
    bool isNegedge,
    const std::vector<RTLIL::SigSig> &syncActions) {
  Instruction instr;
  instr.statement = S_Connect;
  int newWire = driverSig;
  if (isNegedge) {
    SigAssign sa;
    newWire = generateGenWire(-1);
    sa.lhs = getSignal(newWire);
    sa.lhs->isUsed = true;
    sa.op1.sig = getSignal(driverSig);
    sa.op1.sig->isUsed = true;
    sa.func = O_Not;
    tempAssigns.push_back(sa);
  }
  int driver = makeDriverSignal(newWire);
  redefPorts(syncActions, driver);
  instr.connects = tempAssigns;
  curModule.instructions.push_back(instr);
}

void YosysConverterFirrtl::makeRstSignal(
    int driverSig,
    bool isLevel0,
    const std::vector<RTLIL::SigSig> &syncActions) {
  Instruction instr;
  instr.statement = S_Connect;
  SigAssign sa;
  int newWire = generateGenWire(-1);
  Signal *lhs = getSignal(newWire);
  lhs->type = T_AsyncReset;
  sa.lhs = lhs;
  sa.lhs->isUsed = true;
  int op1 = driverSig;
  if (isLevel0) {
    SigAssign sa;
    sa.func = O_Not;
    op1 = generateGenWire(-1);
    sa.lhs = getSignal(op1);
    sa.lhs->isUsed = true;
    sa.op1.sig = getSignal(driverSig);
    sa.op1.sig->isUsed = true;
    tempAssigns.push_back(sa);
  }
  sa.op1.sig = getSignal(op1);
  sa.op1.sig->isUsed = true;
  sa.func = O_AsAsyncReset;
  tempAssigns.push_back(sa);
  redefRstPorts(syncActions, newWire);
  instr.connects = tempAssigns;
  curModule.instructions.push_back(instr);
}

void YosysConverterFirrtl::makeListBlockedRHS(
    const std::vector<RTLIL::SigSig> &actions) {
  for (const auto &[sig1, sig2]: actions) {
    if (isChunk(sig1, sig2)) {
      tmpBlockedRHS.push_back(deterSigSpec(sig1));
    }
  }
}

void YosysConverterFirrtl::walkSyncRule(
    std::vector<RTLIL::SyncRule *> &syncs) {
  for (const auto &sync: syncs) {
    debug << fmt::format("   Sync:\n    Type: {}\n    Signal: {}\n",
                         determineSyncType(sync->type),
                         determineSigSpec(sync->signal));
    int driverSig = deterSigSpec(sync->signal);
    buildAssigns(driverSig);
    debug << "    Actions:\n";
    std::string syncType = determineSyncType(sync->type);
    if (syncType == "negedge") {
      makeClockSignal(driverSig, true, sync->actions);
    } else if (syncType == "posedge") {
      makeClockSignal(driverSig, false, sync->actions);
    } else if (syncType == "level1") {
      makeRstSignal(driverSig, false, sync->actions);
    } else if (syncType == "level0") {
      makeRstSignal(driverSig, true, sync->actions);
    }
    std::map<int, std::vector<RhsDeps>> lhsProc;
    walkActions(sync->actions, true);
    Instruction instr;
    instr.connects = tempAssigns;
    instr.statement = S_Connect;
    curModule.instructions.push_back(instr);
    tempAssigns.clear();
    makeListBlockedRHS(sync->actions);
    tempAssigns.clear();
  }
}

void YosysConverterFirrtl::walkProcesses(
    const IdDict<RTLIL::Process *> &processes) {
  for (const auto &[str, proc]: processes) {
    debug << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    debug << fmt::format(" Process: {} index: {}\n  Syncs:\n",
        readIdString(str),
        str.index_ );
    walkSyncRule(proc->syncs);
    debug << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    debug << "++++++++++++++++++++++++++++++++\n";
    debug << "  Root case:\n";
    walkCaseRule(proc->root_case);
    debug << "++++++++++++++++++++++++++++++++\n";
    debug << " End the process\n";
    tmpBlockedRHS.clear();
  }
}

void YosysConverterFirrtl::walkMemories(
    const IdDict<RTLIL::Memory *> &processes) {
  for (const auto &[str, memory]: processes) {
    fmt::print(debug, "  {} index: {} width: {} start_offset: {} size: {}\n",
        readIdString(str),
        str.index_, memory->width,
        memory->start_offset,
        memory->size);
    Memory mem;
    mem.depth = memory->size;
    mem.widthData = memory->width;
    mem.name = str.str().erase(0, 1);
    curModule.memories.emplace(str.index_, mem);
  }
}

void YosysConverterFirrtl::requireDelayedAssign() {
  Instruction instr;
  instr.statement = S_Connect;
  instr.connects = delayedAssigns;
  delayedAssigns.clear();
  curModule.instructions.push_back(instr);
}

void YosysConverterFirrtl::walkModule(const RTLIL::Module *m) {
  curModule.id = modulesName[m->name.index_];
  debug << "Wires:\n";
  walkWires(m->wires_);
  debug << "End Wires\n\n";
  debug << "Ports:\n";
  walkPorts(m->ports);
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
  debug << "Avail parameters:\n";
  walkParameteres(m->avail_parameters);
  debug << "End Avail parameteres\n\n";
  debug << "Processes:\n";
  walkProcesses(m->processes);
  debug << "End processes\n\n";
  requireDelayedAssign();
  finalModules.push_back(curModule);
  Module defaultModule;
  curModule = defaultModule;
  tempAssigns.clear();
}

std::string YosysConverterFirrtl::getPinModeName(PinMode mode) {
  switch (mode) {
  case PM_Input:
    return "input";
  case PM_Output:
    return "output";
  case PM_Wire:
    return "wire";
  case PM_Reg:
    return "reg";
  case PM_Regreset:
    return "regreset";
  default:
    return "unknown";
  }
}

std::string YosysConverterFirrtl::getTypeName(Type type) {
  switch (type) {
  case T_UInt:
    return "UInt";
  case T_SInt:
    return "SInt";
  case T_Clock:
    return "Clock";
  case T_Reset:
    return "Reset";
  case T_AsyncReset:
    return "AsyncReset";
  default:
    return "Unknown";
  }
}

YosysConverterFirrtl::Operator YosysConverterFirrtl::logicFunction(int type) {
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
  if (type == ID($or).index_ ||
      type == ID($logic_or).index_) {
    return O_Or;
  }
  if (type == ID($reduce_or).index_ ||
      type == ID($reduce_bool).index_) {
    return O_Orr;
  }
  if (type == ID($reduce_and).index_) {
    return O_Andr;
  }
  if (type == ID($reduce_xor).index_) {
    return O_Xorr;
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
  if (type == ID($div).index_) {
    return O_Div;
  }
  if (type == ID($neg).index_) {
    return O_Neg;
  }
  if (type == ID($ne).index_) {
    return O_Neq;
  }
  if (type == ID($eq).index_) {
    return O_Eq;
  }
  if (type == ID($pos).index_) {
    return O_Pad;
  }
  if (type == ID($memrd).index_ ||
      type == ID($memrd_v2).index_) {
    return O_Memrd;
  }
  if (type == ID($memwr).index_ ||
      type == ID($memwr_v2).index_) {
    return O_Memwr;
  }
  if (type == ID($xnor).index_) {
    return O_Xnor;
  }
  if (type == ID($nor).index_) {
    return O_Nor;
  }
  if (type == ID($nand).index_) {
    return O_Nand;
  }
  assert(false && "Unsupport operator ");
  return O_Not;
}
