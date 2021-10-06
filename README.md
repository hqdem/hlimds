# Utopia EDA

Utopia is an open-source HLS-based EDA for digital hardware design.

The EDA takes the following inputs:
* an algorithmic description of the accelerator (IP core);
* a configuration of the target hardware (FPGA, ULA, or ASIC);
* custom constraints.

And produces the following outputs:
* an RTL model of the accelerator;
* recommendations for placing elements of the RTL model on a chip;
* implementation of the API for interacting with the accelerator (if necessary).

## Licensing and Distribution

Utopia is distributed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

## System Requirements

* `gcc`
* `clang`
* `lld`
* `clang-tidy`
* `make`
* `ninja`
* `cmake`
* `flex`
* `bison`

### LLVM Installation

```
cd <workdir>
git clone https://github.com/llvm/llvm-project.git
mkdir llvm-project/build
cd llvm-project/build
cmake -G Ninja ../llvm \
   -DLLVM_ENABLE_PROJECTS=mlir \
   -DLLVM_BUILD_EXAMPLES=ON \
   -DLLVM_TARGETS_TO_BUILD="X86;NVPTX;AMDGPU" \
   -DCMAKE_BUILD_TYPE=Release \
   -DLLVM_ENABLE_ASSERTIONS=ON \
   -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DLLVM_ENABLE_LLD=ON

cmake --build . --target check-mlir
```

```
export MLIR_HOME=<workdir>/llvm-project/build/lib/cmake/mlir/
```

## Working in Command Line

### Building/Running Project

```
cd src
cmake -S . -B build -G Ninja
cmake --build build
./build/umain <file(s)>
```

### Building/Running Unit Tests

```
cd test
cmake -S . -B build -G Ninja
cmake --build build
./build/utest
```

## Working in Visual Studio Code

### Installing VS Studio w/ C/C++ and CMake Extensions
* Download the VS Studio package
  * Go to https://code.visualstudio.com/docs/?dv=linux64_deb
  * Wait until the package is downloaded
* Install the VS Studio package
  ```
  sudo apt install -f ~/Downloads/code_1.60.2-1632313585_amd64.deb
  ```
* Install the C/C++ and CMake extensions
  * Start VS Code
  * Press the `Ctrl+Shift+x` key combination
    * Find and install the `C/C++` extension
    * Find and install the `CMake Tools` extension
  * Click on the `No Kit Selected` text in the status bar
    * Select the kit to use (for example, `GCC 9.3.0 x86_64-linux-gnu`)

### Opening/Building Project

* Click on the `File` and `Open Folder...` menu items
  * Select the `<UTOPIA_HOME>/src` directory
  * Press the `I trust the authors` button
* Click on the `Build` text in the status bar
