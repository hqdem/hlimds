#!/bin/sh

# Run project's test suites excluding longest ones

export GTEST_FILTER="-VerilogFirSys*:\
VerilogTranslatorFirrtl*:\
VerilogTranslatorModel2*:\
Mutator*:\
ReedMuller*:\
BiDecompositionTest*:\
TechmapPowerTest*:\
TechmapAreaRecoveryTest*:\
TechmapAreaTest*:\
DelayEstmt*:\
ReadLibertyTest*:\
TechmapDelayTest*"

./build/test/utest
