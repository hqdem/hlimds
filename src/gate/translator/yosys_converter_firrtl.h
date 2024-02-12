//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright <2023> ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "gate/translator/firrtl.h"

#include <kernel/yosys.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

/**
 * The class encapsulates Yosys-based Verilog frontend and provides
 * utilities to translate an input to FIRRTL. Instances of the class
 * initialize the Yosys framework on construction and finalize it on
 * destruction. Yosys has issues preventing to use it repeatedly
 * after shutdown.
 */

class YosysConverterFirrtl {

  template<typename T>
  using IdDict = Yosys::hashlib::dict<Yosys::RTLIL::IdString, T>;

  std::ostream &outputFile;
  std::ostream &debug;
  std::fstream file;

  static std::ostream devnull;

  enum PinMode {
    PM_Input,
    PM_Output,
    PM_Wire,
    PM_Reg,
    PM_Regreset
  };

  enum Statement {
    S_Inst,
    S_When,
    S_Skip,
    S_Connect,
    S_Dff,
    S_Adff,
    S_Memory
  };

  enum Type {
    T_UInt,
    T_SInt,
    T_Clock,
    T_Reset,
    T_AsyncReset
  };

  struct Signal {
    PinMode mode;
    std::string id;
    Type type;
    int width = 0;
    bool isConst = false;
    bool isDecl = false;
    bool isInvalid = false;
    bool isUsed = false;
    std::string mean;
    std::string driverSig;
    std::string resetSig;
    std::string resetMean;
  };

  enum TypeController {
    TC_Reader,
    TC_Writer,
    TC_ReadWriter
  };

  struct Controller {
    std::string name;
    Signal *en = nullptr;
    Signal *addr = nullptr;
    Signal *clk = nullptr;
    Signal *data = nullptr;
    Signal *mask = nullptr;
    TypeController type;
  };

  struct Memory {
    std::string name;
    size_t widthData;
    size_t depth;
    size_t readLatency = 0;
    size_t writeLatency = 1;
    std::vector<Controller> controllers;
  };

  enum Operator {
    O_Add,
    O_Sub,
    O_Mul,
    O_Div,
    O_Not,
    O_And,
    O_Or,
    O_Orr,
    O_Andr,
    O_Xorr,
    O_Xor,
    O_Cat,
    O_Mux,
    O_Assign,
    O_Shr,
    O_Shl,
    O_Dshr,
    O_Dshl,
    O_Leq,
    O_Lt,
    O_Geq,
    O_Gt,
    O_Neg,
    O_Neq,
    O_Eq,
    O_Bits,
    O_Pad,
    O_AsClock,
    O_AsAsyncReset,
    O_Memrd,
    O_Memwr,
    O_Nor,
    O_Nand,
    O_Xnor
  };

  struct Operand {
    Signal *sig = nullptr;
    int hi = -1;
    int lo = 0;
  };

  struct SigAssign {
    Signal *lhs;
    Operand op1;
    Operand op2;
    Operand op3;
    Operator func;
  };

  enum CondKeyWord {
    CKW_If,
    CKW_Else
  };

  struct CondStatement {
    std::vector<CondKeyWord> branch;
    Signal *sig = nullptr;
    std::vector<SigAssign> connects;
  };

  struct DataPorts {
    std::pair<Signal *, Signal *> signals;
    std::pair<int, int> params;
  };

  struct Instance {
    std::string nameNestedModule;
    std::string idInstance;
    std::vector<DataPorts> ports;
    int indexNestedModule;
  };

  struct Instruction {
    Statement statement;
    std::vector<SigAssign> connects;
    std::vector<CondStatement> branches;
    Instance instance;
  };

  struct RhsOperands {
    std::vector<int> indexOperands;
    std::vector<std::pair<int, int>> parmsOperands;
  };

  struct RhsDeps {
    int indexRhs;
    int bitRhs;
    int bitLhs;
    int bitLhsLo;
    int bitLhsHi;
    int bitRhsLo;
    int bitRhsHi;
  };

  struct FlipFlop {
    int clk, data, lhs, out, rst;
    std::pair<int, int> offsetDataPort;
  };

  struct Module {
    std::string id;
    std::map<int, Signal *> signals;
    std::map<int, Memory> memories;
    std::vector<Signal *> genSig;
    std::vector<Instruction> instructions;

    std::map<int, RhsOperands> yosysCells;
    std::map<int, std::vector<RhsDeps>> depsLHS;
    std::map<int, Operator> operators;
    int newIndex = -1;
    int indexModule;
  };

  Module curModule;
  std::vector<Module> tmpModules;
  std::vector<SigAssign> tempAssigns;
  std::vector<SigAssign> tempCases;
  std::vector<SigAssign> delayedAssigns;
  CondStatement tempCondStatement;
  std::vector<Module> finalModules;
  std::string nameTopModule;
  std::vector<int> tmpBlockedRHS;

  using DependentLhs = std::map<int, std::vector<RhsDeps>>;

  struct LhsStack {
    DependentLhs lhs;
    std::vector<DependentLhs> stack;

    void popBack() {
      if (stack.size() == 1) {
        DependentLhs newLhs;
        lhs = newLhs;
      } else if (stack.size() > 1) {
        stack.pop_back();
        lhs = stack.back();
      } else {
        assert(0 && "Trying popBack() of empty Stack");
      }
    }

    void pushBack(DependentLhs newDependsLhs) {
      stack.push_back(newDependsLhs);
      lhs = newDependsLhs;
    }
  };

  LhsStack stackLHS;

  std::map<int, Yosys::RTLIL::Module *> modules;
  std::map<int, std::string> modulesName;
  std::map<std::pair<int, std::string>, PinMode> portsMode;

  std::string genName = "_GEN_";
  size_t numbGenName = 0;

public:

  YosysConverterFirrtl(const FirrtlConfig &opts);

  void deterTopModule(
    Yosys::RTLIL::Design &design, const std::string &topModule);

  static void declareSignal(std::ostream &os, const Signal &sig);

  void declareMemory(std::ostream &os, const Memory &memory);

  void declareModule(std::ostream &os, const Module &circuit);

  static bool isMean(const std::string &mean);

  static std::string getNameSignal(const Signal *sig);

  static std::string makeOperandPrint(const Operand &instr);

  static std::string makeNullary(const SigAssign &instr);

  static std::string makeUnary(const SigAssign &instr);

  static std::string makeBinary(const SigAssign &instr);

  static std::string makeTernary(const SigAssign &instr);

  static void declareConnectInstruction(
      std::ostream &os, const Instruction &instr);

  static void declareWhenInstruction(
      std::ostream &os, const Instruction &instr);

  void declareInstInstruction(std::ostream &os, const Instruction &instr);

  static void declareSigAssign(std::ostream &os, const SigAssign &instr);

  void declareInstruction(std::ostream &os, const Instruction &instr);

  void declareInstructions(
      std::ostream &os, const std::vector<Instruction> &vec);

  ~YosysConverterFirrtl();

private:

  struct RhsOperand {
    int index;
    std::pair<int, int> parm;
  };

  static bool hasInOutMode(Signal *sig);

  static bool isDeclWire(Signal *sig);

  static bool isReg(Signal *sig);

  std::string getName();

  static void countIndent(std::ostream &os, size_t indent);

  static std::string operatorToString(Operator op);

  void makeMapModules(const IdDict<Yosys::RTLIL::Module *> &m_);

  void readModules(const Yosys::RTLIL::Design &des);

  static std::string readIdString(const Yosys::RTLIL::IdString &str);

  static bool hasIllegalSymbols(const std::string &inputStr);

  std::string checkName(std::string wireName);

  void walkWires(const IdDict<Yosys::RTLIL::Wire *> &ywires);

  static bool isChunk(
      const Yosys::RTLIL::SigSpec &first, const Yosys::RTLIL::SigSpec &second);

  static std::string getConst(const Yosys::RTLIL::Const &opConst);

  static int getIdMemory(const Yosys::RTLIL::Const &opConst);

  static std::string getStateString(const Yosys::RTLIL::State state);

  static bool hasDontCareBits(const std::string &const_);

  int genDontCareBits(Signal *sig);

  int generateConstSig(const Yosys::RTLIL::SigSpec &sigWire);

  int generateConst(const std::string &digit);

  void requireOperand(
      Operand &op, int index, int hi, int lo);

  size_t countWidth(int index);

  int generateGenWire(int width);

  int makeCat(const Yosys::RTLIL::SigSpec &sigWire);

  int deterSigSpec(const Yosys::RTLIL::SigSpec &sigWire);

  std::pair<int, int> deterSigSpecBits(const Yosys::RTLIL::SigSpec &sigWire);

  std::string determineSigSpec(const Yosys::RTLIL::SigSpec &sigWire);

  bool isMemoryType(int index);

  Statement determineStatement(int index);

  bool determineClkPolarity(const IdDict<Yosys::RTLIL::Const> &parms_);

  bool determineRstPolarity(const IdDict<Yosys::RTLIL::Const> &parms_);

  int makePolarityDriverSig(bool posedge, int clk);

  int makePolarityRstSig(bool posedge, int rst);

  void makeRenameOutput(int index);

  int deterDffLHS(const Yosys::RTLIL::SigSpec &sig);

  void makeDFF(const Yosys::RTLIL::Cell *cell, bool isAsync = false);

  std::string determineInitValue(const IdDict<Yosys::RTLIL::Const> &parms_);

  void makeADFF(const Yosys::RTLIL::Cell *cell);

  void makeInstance(
      const int typeFunction,
      const IdDict<Yosys::RTLIL::SigSpec> &cons,
      const std::string &nameInst);

  static std::string binaryToDecimal(const std::string &binaryStr);

  void checkShiftOperator(Operator &operator_, RhsOperands &leafs);

  void checkPad(
      int firstParms, Operator operator_, RhsOperands &leafs);

  void makeUnaryConnect(
      const int typeFunction,
      const IdDict<Yosys::RTLIL::SigSpec> &cons);

  void makeBinaryConnect(
      const int typeFunction,
      const IdDict<Yosys::RTLIL::SigSpec> &cons);

  void printCell(const Yosys::RTLIL::Cell *cell);

  Memory *getMemory(
      const IdDict<Yosys::RTLIL::Const> &parameters);

  bool isReadMemory(int index);

  bool isWriteMemory(int index);

  void fillMask(Memory *mem, Instruction &instr, Controller &controller);

  void makeController(const Yosys::RTLIL::Cell *cell, Memory *mem);

  TypeController determineTypeController(const int index);

  void makeMemory(const Yosys::RTLIL::Cell *cell);

  void unifyRhsOperands(
      RhsOperands &leafs,
      const std::vector<RhsOperand> &leaf_);

  void insertData(
      const Yosys::RTLIL::SigChunk &chunk,
      const int lhsWire,
      const int length);

  void printDeps();

  int deterLengthLhs(const Yosys::RTLIL::SigSpec &lhs);

  int buildLHS(const Yosys::RTLIL::SigSpec &lhs);

  void makeMux(const IdDict<Yosys::RTLIL::SigSpec> &cons);

  bool isMux(const int index);

  void walkCells(const IdDict<Yosys::RTLIL::Cell *> &ycells);

  static bool isUnOperator(const Operator func);

  static bool isBinOperator(const Operator func);

  static bool isTernOperator(const Operator func);

  static bool isSpecifiedOperator(const Operator func);

  static int determineTypeOperator(const Operator func);

  void buildAssigns(int root, bool isInvalid = false);

  Signal *getSignal(const int index);

  void printAllYosysCells();

  void walkAllYosysCells();

  static bool compareByBitLhsLo(const RhsDeps &lhs1, const RhsDeps &lhs2);

  void makeCatRhs(int indexLhs, const std::vector<RhsDeps> &vec);

  void walkDepsLhs();

  void walkConnections(const std::vector<Yosys::RTLIL::SigSig> &cons);

  void printConnections(
      const Yosys::RTLIL::SigSpec &op1,
      const Yosys::RTLIL::SigSpec &op2);

  void walkParameteres(
      const Yosys::hashlib::idict<Yosys::RTLIL::IdString> &availParms);

  void walkPorts(const std::vector<Yosys::RTLIL::IdString> &ports);

  bool isSigSpec(const int op1, const int op2);

  bool isUndef(
      const Yosys::RTLIL::SigSpec &sig1, const Yosys::RTLIL::SigSpec &sig2);

  void determineInvalid(Signal *sig);

  static bool containsIndex(const std::vector<int>& vec, int index);

  void fillLhsProc(
      const int lhs,
      const int rhs,
      const std::pair<int, int> parmsLhs,
      int &curBit,
      std::map<int, std::vector<RhsDeps>> &lhsProc);

  static bool compareDataByBitLhs(const RhsDeps &a, const RhsDeps &b);

  static void removeElementsByIndices(
      std::vector<RhsDeps> &targetVector,
      const std::vector<int> &indicesToRemove);

  void keepLastElementsByBitLhs(std::vector<RhsDeps> &dataVector);

  void makeAssign(const int lhs, const std::vector<RhsDeps> &vec);

  void makeLhs(
      const Yosys::RTLIL::SigSpec &sig,
      const Yosys::RTLIL::SigSpec &sigRhs,
      std::map<int, std::vector<RhsDeps>> &lhsProc);

  void declareOperand(const int op, const bool SA = false);

  void walkActions(
      const std::vector<Yosys::RTLIL::SigSig> &actions,
      bool isInvalid = false);

  static std::vector<int> findDontCareBits(std::string &value);

  int makeCatWire(std::vector<int> &vec, bool isInvalid = false);

  int assignBit(int op, int bit);

  std::pair<int, int> makeCat(int op2_, int op1_, std::vector<int> &indices);

  std::pair<int, int> giveDontCareBits(int op2_, int op1_);

  int makeCondSignal(const Yosys::RTLIL::SigSpec &signal);

  void fillParameters(
      const Yosys::RTLIL::SigSpec &op, int &hi, int &lo);

  int makeCase(
      const Yosys::RTLIL::SigSpec &op1, const Yosys::RTLIL::SigSpec &op2);

  void requireCases();

  int makeElse(std::vector<int> &switchers);

  void walkSwitches(
      const std::vector<Yosys::RTLIL::SwitchRule *> &switches);

  void walkSimpleCase(
      const Yosys::RTLIL::SigSpec &switch_,
      const Yosys::RTLIL::CaseRule *case_,
      std::vector<int> &switchers,
      Instruction &instr);

  void walkSwitch(
      const std::vector<Yosys::RTLIL::SwitchRule *> &switches,
      Instruction &instr);

  void walkCaseRule(const Yosys::RTLIL::CaseRule &caseRule);

  static std::string determineSyncType(Yosys::RTLIL::SyncType sync_type);

  int makeDriverSignal(int driverSig, SigAssign *sa_ = nullptr);

  void addDelayedAssign(Signal *lhs, Signal *op1);

  int copySignal(Signal *sig);

  void redefPorts(
      const std::vector<Yosys::RTLIL::SigSig> &ports, int driver);

  int makeMyInit(const Yosys::RTLIL::SigSpec &op2);

  void redefRstPorts(
      const std::vector<Yosys::RTLIL::SigSig> &ports, int driver);

  void makeClockSignal(
      int driverSig,
      bool isNegedge,
      const std::vector<Yosys::RTLIL::SigSig> &syncActions);

  void makeRstSignal(
      int driverSig,
      bool isLevel0,
      const std::vector<Yosys::RTLIL::SigSig> &syncActions);

  void makeListBlockedRHS(const std::vector<Yosys::RTLIL::SigSig> &actions);

  void walkSyncRule(std::vector<Yosys::RTLIL::SyncRule *> &syncs);

  void walkProcesses(const IdDict<Yosys::RTLIL::Process *> &processes);

  void walkMemories(const IdDict<Yosys::RTLIL::Memory *> &processes);

  void requireDelayedAssign();

  void walkModule(const Yosys::RTLIL::Module *m);

  static std::string getPinModeName(PinMode mode);

  static std::string getTypeName(Type type);

  static Operator logicFunction(int type);

  int makeCatSigSpec(const Yosys::RTLIL::SigSpec &sigWire);

  static int findDataByBitLhsHi(
      const std::vector<RhsDeps> &vec, int searchValue);

  int deterSigSpecRHS(const Yosys::RTLIL::SigSpec &sigWire);

  std::pair<int, int> deterSigSpecBitsRHS(const Yosys::RTLIL::SigSpec &sigWire);

  void createOutputFile(const std::string &outputNamefile);

  int makePolaritySig(bool posedge, int clk);

  FlipFlop fillPortsDFF(bool isAsync, const IdDict<Yosys::RTLIL::SigSpec> &cons);

  const char *SID_A = "\\A";
  const char *SID_ADDR = "\\ADDR";
  const char *SID_ARST = "\\ARST";
  const char *SID_ARST_POLARITY = "\\ARST_POLARITY";
  const char *SID_ARST_VALUE = "\\ARST_VALUE";
  const char *SID_B = "\\B";
  const char *SID_CLK = "\\CLK";
  const char *SID_CLK_POLARITY = "\\CLK_POLARITY";
  const char *SID_D = "\\D";
  const char *SID_DATA = "\\DATA";
  const char *SID_EN = "\\EN";
  const char *SID_MEMID = "\\MEMID";
  const char *SID_S = "\\S";
  const char *SID_Q = "\\Q";
  const char *SID_Y = "\\Y";
};
