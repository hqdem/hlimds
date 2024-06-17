[//]: <> (SPDX-License-Identifier: Apache-2.0)

# Utopia EDA

Utopia is an EDA for logic synthesis of digital hardware designs.

## Licensing and Distribution

Utopia is distributed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

## Coding Style

See `doc/CodeStyle.md` for more details about our coding convention.

## General Notes

See `doc/Notes.md` if you're not familiar with program building/installing on Linux.

## System Requirements

The recommended operating system for Utopia is Ubuntu 20.04. The package names
below are specific to this operating system:

* `autoconf`
* `bison`
* `build-essential`
* `clang`
* `clang-tidy`
* `cmake`
* `doxygen`
* `flex`
* `g++`
* `gcc`
* `graphviz`
* `gtkwave`
* `iverilog`
* `libfmt-dev`
* `liblpsolve55-dev`
* `libssl-dev`
* `libtool`
* `lld`
* `make`
* `ninja-build`
* `pkg-config`
* `python`
* `zlib1g`
* `zlib1g-dev`

To install them, do the following:
```
sudo apt install autoconf bison build-essential clang clang-tidy cmake doxygen \
    flex g++ gcc graphviz gtkwave iverilog libfmt-dev liblpsolve55-dev \
    libssl-dev libtool lld make ninja-build pkg-config python zlib1g zlib1g-dev
```

If you are working on Fedora Linux OS, please, follow this [guide](doc/Fedora.md) to build Utopia EDA.

### CMake Installation

```
cd <workdir>
wget https://cmake.org/files/v3.28/cmake-3.28.1.tar.gz
tar xzf cmake-3.28.1.tar.gz
rm -rf cmake-3.28.1.tar.gz
cd cmake-3.28.1
./bootstrap
make
sudo make install
```
If you prefer to install CMake as package, please see this [guide](https://apt.kitware.com).

### C++ CTemplate Installation

```
cd <workdir>
git clone https://github.com/OlafvdSpek/ctemplate.git
cd ctemplate
./autogen.sh
./configure --prefix=/usr
make
sudo make install
```
If you would like to install CTemplate to a non-standard location, please
specify `--prefix` option of `configure` script to installation directory
you want and set `CT_DIR` environment variable to it too.

### CUDD Installation

```
cd <workdir>
git clone https://github.com/ivmai/cudd
cd cudd
touch aclocal.m4 Makefile.am Makefile.in configure
./configure --enable-obj --enable-shared
make
sudo make install
```

If you want to install `CUDD` not in default directory by using
`--prefix` option of configure script, then building `Utopia EDA`
will require environment variable `CUDD_DIR` that contains the path
to the `CUDD` actual installation directory.

### STACCATO Installation

The `<path_to_cudd_dir>` refers to the path to the CUDD sources directory.

```
cd <workdir>
git clone https://github.com/ispras/staccato
cd staccato
make BUILD_TYPE=shared CUDD_INCLUDE=<path_to_cudd_dir> SM="-DDISABLE_SM"
sudo make install
```

### Configuring with `Yosys`

1. Get `Yosys` version 0.36 source code from the [^yosys] into `<yosys-dir>`.
2. Make sure your system meets the requirements listed in `<yosys-dir>/README.md`
3. Edit `<yosys-dir>/Makefile`
    - set `ENABLE_LIBYOSYS` to 1
4. Build and install `Yosys` as described in the `<yosys-dir>/README.md`
5. Configure `Utopia` to find `Yosys`
    - add `-DYosys_ROOT=<yosys-dir>` to the `cmake` invocation
    - e.g. `cmake -S <utopia-source-dir> -B <utopia-build-dir> -DYosys_ROOT=<yosys-dir> -G Ninja`

[^yosys]: https://github.com/YosysHQ/yosys/tree/yosys-0.36

### CIRCT Installation

To configure the build process using a CIRCT binary distribution avoiding the
need to build from the source, see `doc/CirctPrebuilt.md`.

LLVM requires a significant amount of RAM (about 8 Gb or more) to build.
Please take this into account while moving through the guide.

#### Check out LLVM and CIRCT repos

```
cd <workdir>
git clone https://github.com/circt/circt.git
cd circt
git checkout firtool-1.72.0
git submodule init
git submodule update
```

#### LLVM/MLIR Installation

Set `MLIR_DIR` environment variable to the directory with MLIR CMake files:
```
export MLIR_DIR=<workdir>/circt/llvm/build/lib/cmake/mlir/
```

Type the following commands:
```
cd <workdir>/circt
mkdir llvm/build
cd llvm/build
cmake -G Ninja ../llvm \
    -DLLVM_ENABLE_PROJECTS="mlir;clang;clang-tools-extra;lld" \
    -DLLVM_TARGETS_TO_BUILD="X86" \
    -DCMAKE_BUILD_TYPE="Release" \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DLLVM_ENABLE_LLD=ON \
    -DLLVM_PARALLEL_LINK_JOBS=$(nproc) \
    -DLLVM_PARALLEL_COMPILE_JOBS=$(nproc)
ninja
```

#### CIRCT Installation

Set `CIRCT_DIR` environment variable to the directory with CIRCT CMake files:
```
export CIRCT_DIR=<workdir>/circt/build/lib/cmake/circt/
```

Type the following commands:
```
cd <workdir>/circt
mkdir build
cd build
cmake -G Ninja .. \
    -DCMAKE_BUILD_TYPE="Release" \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DMLIR_DIR=$PWD/../llvm/build/lib/cmake/mlir \
    -DLLVM_DIR=$PWD/../llvm/build/lib/cmake/llvm \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DVERILATOR_DISABLE=ON \
    -DIVERILOG_DISABLE=ON
ninja
```

## Working in Command Line

### Clone Project Repository and Set Environment Variable

```
cd <workdir>
git clone --recursive https://gitlab.ispras.ru/mvg/utopia-eda.git
cd utopia-eda
export UTOPIA_HOME=<workdir>/utopia-eda
export Yosys_ROOT=<workdir>/yosys
```
Please keep `UTOPIA_HOME` and `Yosys_ROOT` variables and the values in your system permanently.

### Project Building

```
cd utopia-eda
cmake -S . -B build -G Ninja
cmake --build build
```
or simply run the following script:
```
./build.sh
```
If you've modified some of the project files, you can use `rebuild.sh` script
for incremental build.

Several additional options can be activated during build process. By default
they are disabled. You may enable them to have access to auxiliary facilities.
Here they are (see `debug-build.sh` as example):

* `GEN_DOC` &mdash; documentation generation (the result is stored at `build/doc`);
* `NPN4_USAGE_STATS` &mdash; statistics for 4-in cones distribution on NPN classes;
* `UTOPIA_DEBUG` &mdash; extended debug printing.

### Running Utopia EDA

```
rm -rf $UTOPIA_HOME/output
./build/src/umain rtl <file(s)> <options>
```
To list the Utopia EDA options, do the following:
```
./build/src/umain --help-all
```

### Running Verilog-to-FIRRTL Translator
```
./build/src/umain to_firrtl <file(s)>
```

When selecting this option, you must specify the path to the Verilog file(s) (`file(s)`).
The results of the translation will be in the standard output file.
(Top module of the descriptions will be determined automatically).

```
./build/src/umain to_firrtl <file(s)> --top <module-name>
```

When selecting this option, you must specify the name of the top module
(`module-name`) in the Verilog file(s) and the path to the file(s) itself (`file(s)`).
The results of the translation will be as the first example.

```
./build/src/umain to_firrtl <file(s)> --output <namefile>
```

When selecting this option, you must specify the name of file (`namefile`), where will be result of
the translation. The file will be placed in same directory with the application.

```
./build/src/umain to_firrtl <file(s)> --verbose
```
To translate input description into inner representation (so called model2), you must specify:

When selecting these option, debug information will be generated in standart error output file.

### Running Verilog-to-Model2 Translator
The translator is used to translate input description into inner representation (so called model2).

```
./build/src/umain verilog_to_model2 <file(s)>
```

When selecting this option, you must specify the path to the Verilog file(s) (`file(s)`).
(Top module of the descriptions will be determined automatically).

```
./build/src/umain verilog_to_model2 <file(s)> --top <module-name>
```

When selecting this option, you must specify the name of the top module
(`module-name`) in the Verilog file(s) and the path to the file(s) itself (`file(s)`).

```
./build/src/umain verilog_to_model2 <file(s)> --verbose
```

When selecting these option, debug information will be generated in standart error output file.

#### Running Translator to Model2

```
./build/src/umain to_model2 <in-file-1> <in-file-2> ... <in-file-n> \
    --net <out-file> --top <name> --fir <fir-file> --verbose
```
To translate input description into inner representation (so called model2), you
must specify:

1. (Necessary) the input description (`<in-file-1> <in-file-2> ... <in-file-n>`,
FIRRTL and Verilog are supported)*;
2. (Optional) for Verilog output model2 representation -- the path to output
file (`--net <out-file>`);
3. (Optional) for Verilog input descriptions -- top module name
(`--top <name>`);
4. (Optional) for Verilog input descriptions -- FIRRTL intermediate file
(`--fir <fir-file>`);
5. (Optional) for Verilog input descriptions you can also specify additional
debug print. (`--verbose`).

*NOTE: For FIRRTL files n == 1

### Tests Running

#### All Tests Running

```
rm -rf $UTOPIA_HOME/output
$UTOPIA_HOME/build/test/utest
```
or
```
./run-tests.sh
```

#### Specific Tests Running

```
./build/test/utest --gtest_filter=<test-pattern>
```
or
```
./filter-tests.sh <test-pattern>
```
Test pattern accepts ```*``` and ```?``` wildcards.

To run "shortest" test suites (which take less than 10 min) use `smoke-tests.sh`.

#### List Available Tests

```
./build/test/utest --gtest_list_tests
```