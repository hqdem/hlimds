//===----------------------------------------------------------------------===//
//
// Part of the Utopia EDA Project, under the Apache License v2.0
// SPDX-License-Identifier: Apache-2.0
// Copyright 2023-2024 ISP RAS (http://www.ispras.ru)
//
//===----------------------------------------------------------------------===//

#include "util/env.h"

#include "gtest/gtest.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace eda::gate::translator {
const std::string homePath = eda::env::getHomePathAsString();
const std::string binPath = homePath + "/build/src/umain";
const std::string dataPath = homePath + "/test/data/gate/parser/verilog/";
const std::string outputFilePath = homePath + "/output/test/verilog_fir_sys/";
const std::string scriptPath = homePath + "/test/gate/translator/verilog_to_fir/test.tcl";

bool exists(const std::string &filename) {
  std::ifstream file(filename);
  return file.good() && file.peek() != std::ifstream::traits_type::eof();
}

inline void createDir(fs::path path) {
  if (!(fs::exists(path) && fs::is_directory(path))) {
    fs::create_directory(path);
  }
}

void createOutputDir() {
  fs::path outDir = fs::path(homePath) /
  "output";
  createDir(outDir);
  outDir = fs::path(outDir / "test");
  createDir(outDir);
  outDir = fs::path(outDir / "verilog_fir_sys");
  createDir(outDir);
}

bool cmpFiles(const std::string &filename1, const std::string &filename2) {
  auto optn = std::ios::binary | std::ios::ate;
  std::ifstream file1(filename1, optn);
  std::ifstream file2(filename2, optn);
  char ch1, ch2;
  while (file1.get(ch1) && file2.get(ch2)) {
    if (ch1 != ch2) {
      return false;
    }
  }
  return file1.eof() && file2.eof();
}

void setDataPath(const std::string &filename, std::string &data,
                 std::string &outputFile) {
  data = dataPath + filename + ".v";
  outputFile = outputFilePath + filename + "_output.fir";
}

int toFirrtl(const std::string &in,
              const std::string data,
              const std::string &out) {

  createOutputDir();
  std::string command = binPath + " -s " +
      scriptPath + " " + data + " " + out;
  const int res = system(command.c_str());
  if (res != 0) {
    std::cerr << command << std::endl;
    std::cerr << "Error occurred: " << strerror(errno) << std::endl;
  }
  return res;
}

bool checkPassed(const std::string &filename) {
  std::string data, outputFile;
  setDataPath(filename, data, outputFile);
  const int res = toFirrtl(filename, data, outputFile);
  
  return !res && exists(outputFile);
}

bool checkFailed(const std::string &filename) {
  std::string data, outputFile;
  setDataPath(filename, data, outputFile);
  const int res = toFirrtl(filename, data, outputFile);
  
  return res;
}

bool checkPassedVerbose(const std::string &filename) {
  std::string data, outputFile;
  setDataPath(filename, data, outputFile);
  std::string command = binPath + " -s " +
      scriptPath + " " + data + " " + outputFile +
          " --verbose > " + outputFile + " 2>&1";
  const int res = system(command.c_str());
  if (res != 0) {
    std::cerr << "Error occurred: " << strerror(errno) << std::endl;
  }
  return !res && exists(outputFile);
}

bool firtoolCheck(const std::string &outputFile) {
  std::string firTool = eda::env::getValue("CIRCT_DIR");
  std::string cutoff = "/lib/cmake/circt/";

  if (firTool.find("circt/") == std::string::npos) {
    std::cout << "CIRCT_DIR env var isn't or incorreclty set!" << std::endl;
    exit(1);
  } else {
    if (firTool.find(cutoff) !=  std::string::npos) {
      firTool.erase(firTool.end() - cutoff.length(), firTool.end());
      firTool = std::string(firTool + "/bin/firtool ");
    } else {
      firTool = std::string(firTool + "build/bin/firtool ");
    }
  }

  std::string pathToFirFile = std::string(homePath +
  "/output/test/verilog_fir_sys/" + outputFile + "_output.fir");

  std::string outputVerilogFile = std::string(homePath +
  "/output/test/verilog_fir_sys/" + outputFile + "_verilog.v");

  std::string command = firTool + pathToFirFile + " > " + outputVerilogFile;
  const int res = system(command.c_str());
  if (res != 0) {
    std::cerr << "Error occurred: " << strerror(errno) << std::endl;
  }
  return !res && exists(outputVerilogFile);
}

TEST(VerilogFirSys, Adder) {
  EXPECT_TRUE(checkPassed("adder") && firtoolCheck("adder"));
}

TEST(VerilogFirSys, Arbiter) {
  EXPECT_TRUE(checkPassed("arbiter") && firtoolCheck("arbiter"));
}

TEST(VerilogFirSys, Bar) {
  EXPECT_TRUE(checkPassed("bar") && firtoolCheck("bar"));
}

TEST(VerilogFirSys, C17) {
  EXPECT_TRUE(checkPassed("c17") && firtoolCheck("c17"));
}

TEST(VerilogFirSys, C17_modified) {
  EXPECT_TRUE(checkFailed("c17_modified"));
}

TEST(VerilogFirSys, C432) {
  EXPECT_TRUE(checkPassed("c432") && firtoolCheck("c432"));
}

TEST(VerilogFirSys, C499) {
  EXPECT_TRUE(checkPassed("c499") && firtoolCheck("c499"));
}

TEST(VerilogFirSys, C880) {
  EXPECT_TRUE(checkPassed("c880") && firtoolCheck("c880"));
}

TEST(VerilogFirSys, C1355) {
  EXPECT_TRUE(checkPassed("c1355") && firtoolCheck("c1355"));
}

TEST(VerilogFirSys, C1908) {
  EXPECT_TRUE(checkPassed("c1908") && firtoolCheck("c1908"));
}

TEST(VerilogFirSys, C2670) {
  EXPECT_TRUE(checkPassed("c2670") && firtoolCheck("c2670"));
}

TEST(VerilogFirSys, C3540) {
  EXPECT_TRUE(checkPassed("c3540") && firtoolCheck("c3540"));
}

TEST(VerilogFirSys, C5315) {
  EXPECT_TRUE(checkPassed("c5315") && firtoolCheck("c5315"));
}

TEST(VerilogFirSys, C6288) {
  EXPECT_TRUE(checkPassed("c6288") && firtoolCheck("c6288"));
}

TEST(VerilogFirSys, C7552) {
  EXPECT_TRUE(checkPassed("c7552") && firtoolCheck("c7552"));
}

TEST(VerilogFirSys, Cavlc) {
  EXPECT_TRUE(checkPassed("cavlc") && firtoolCheck("cavlc"));
}

TEST(VerilogFirSys, Ctrl) {
  EXPECT_TRUE(checkPassed("ctrl") && firtoolCheck("ctrl"));
}

TEST(VerilogFirSys, Dec) {
  EXPECT_TRUE(checkPassed("dec") && firtoolCheck("dec"));
}

TEST(VerilogFirSys, Div) {
  EXPECT_TRUE(checkPassed("div") && firtoolCheck("div"));
}

TEST(VerilogFirSys, I2c) {
  EXPECT_TRUE(checkPassed("i2c") && firtoolCheck("i2c"));
}

TEST(VerilogFirSys, Int2float) {
  EXPECT_TRUE(checkPassed("int2float") && firtoolCheck("int2float"));
}

TEST(VerilogFirSys, Log2) {
  EXPECT_TRUE(checkPassed("log2") && firtoolCheck("log2"));
}

TEST(VerilogFirSys, Max) {
  EXPECT_TRUE(checkPassed("max") && firtoolCheck("max"));
}

TEST(VerilogFirSys, Memctrl) {
  EXPECT_TRUE(checkPassed("memctrl") && firtoolCheck("memctrl"));
}

TEST(VerilogFirSys, Multiplier) {
  EXPECT_TRUE(checkPassed("multiplier") && firtoolCheck("multiplier"));
}

TEST(VerilogFirSys, Priority) {
  EXPECT_TRUE(checkPassed("Priority") && firtoolCheck("Priority"));
}

TEST(VerilogFirSys, Router) {
  EXPECT_TRUE(checkPassed("router") && firtoolCheck("router"));
}

TEST(VerilogFirSys, Sin) {
  EXPECT_TRUE(checkPassed("sin") && firtoolCheck("sin"));
}

TEST(VerilogFirSys, Sqrt) {
  EXPECT_TRUE(checkPassed("sqrt") && firtoolCheck("sqrt"));
}

TEST(VerilogFirSys, Square) {
  EXPECT_TRUE(checkPassed("square") && firtoolCheck("square"));
}

TEST(VerilogFirSys, Voter) {
  EXPECT_TRUE(checkPassed("voter") && firtoolCheck("voter"));
}

TEST(VerilogFirSys, VerboseTest) {
  checkPassed("bar");
  const std::string output_path = std::string(homePath +
  "output/test/verilog_fir_sys/bar_verilog.v");
  checkPassedVerbose("bar");
  const std::string output_verb_path = std::string(homePath +
  "output/test/verilog_fir_sys/bar_verbose_verilog.txt");
  EXPECT_TRUE(!(cmpFiles(output_path, output_verb_path)));
}

} // namespace eda::gate::translator
