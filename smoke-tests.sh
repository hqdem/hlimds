#!/bin/sh

# Run project's test suites excluding longest ones

export GTEST_FILTER="-VerilogTranslatorFirrtl*:\
RilLecTest*:\
VerilogTranslatorModel2*:\
XmgMapperTest*:\
Mutator*:\
MigMapperTest*:\
ReedMuller*:\
BiDecompositionTest*:\
VlogLecTest*:\
TechmapPowerTest*:\
TechmapAreaRecoveryTest*:\
TechmapAreaTest*:\
ParserGraphMLTest*:\
Verilog2GraphMlTest*:\
GateVerilogPrinter*:\
AssociativeBalanceTest*:\
DelayEstmt*:\
ReadLibertyTest*:\
AigPremapperVerilogTest*:\
TechmapDelayTest*:\
RewriteTest*:\
LorinaTest*:\
ArithmeticTest*"

./build/test/utest
