# Utopia EDA

Utopia is an EDA (Electronic Design Autiomation) framework for logic synthesis of digital hardware designs.

## Licensing and distribution

Utopia is distributed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

## Coding style

See `doc/CodeStyle.md` for more details about our coding convention.

## General notes

See `doc/Notes.md` if you're not familiar with program building/installing on Linux.

## Building Utopia EDA

To run Utopia EDA, you should build it either from source, or using Docker.

If you prefer Docker-based solution, use `Dockerfile.local` for that purpose.
See `doc/Docker.md` if you're not familiar with Docker.

To build the project from source on your own machine, see the instruction below.

### System requirements

The recommended operating system for Utopia is **Ubuntu 20.04**. The package names
below are specific to this operating system:

* `autoconf`
* `bison`
* `clang`
* `clang-tidy`
* `doxygen`
* `flex`
* `g++`
* `gcc`
* `git`
* `graphviz`
* `libctemplate-dev`
* `libfmt-dev`
* `libssl-dev`
* `libtool`
* `lld`
* `make`
* `ninja-build`
* `pkg-config`
* `rlwrap`
* `tcl-dev`
* `zlib1g`
* `zlib1g-dev`

To install them, do the following:

```shell
sudo apt install autoconf bison clang clang-tidy doxygen flex g++ gcc git \
     graphviz libctemplate-dev libfmt-dev libssl-dev libtool lld make \
     ninja-build pkg-config rlwrap tcl-dev zlib1g zlib1g-dev
```

If you are working on Fedora Linux OS, see `doc/Fedora.md`.

### CMake installation

For project building we use [CMake](https://cmake.org). The minimum required version for CMake is **3.28**.
Check whether it is available on your OS, and if it is not, see `doc/CMake.md`.

### CUDD installation

To build CUDD library from sources, do the following:

```shell
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

### STACCATO installation

To build STACCATO library from sources, do the following
(here `<cudd-path>` refers to the path to the CUDD sources directory):

```shell
cd <workdir>
git clone https://github.com/ispras/staccato
cd staccato
make BUILD_TYPE=shared CUDD_INCLUDE=<cudd-path> SM="-DDISABLE_SM"
sudo make install
```

### Yosys installation

1. Get `Yosys` version 0.36 source code from [repo](https://github.com/YosysHQ/yosys/tree/yosys-0.36) into `<yosys-dir>`.

   ```shell
   git clone https://github.com/YosysHQ/yosys.git <yosys-dir>
   cd <yosys-dir>
   git checkout yosys-0.36
   ```

2. Make sure your system meets the requirements listed in `<yosys-dir>/README.md`
3. Edit `<yosys-dir>/Makefile`
    * set `ENABLE_LIBYOSYS` to 1
4. Build and install `Yosys` as described in the `<yosys-dir>/README.md`

### MLIR/CIRCT installation

To configure the build process using a CIRCT binary distribution avoiding the
need to build from the source, see `doc/CirctPrebuilt.md`.

To build CIRCT from sources, please follow the instruction below.
LLVM requires a significant amount of RAM (about 8 Gb or more) to build.
Please take this into account while moving through the guide.

#### Check out LLVM and CIRCT repos

```shell
cd <workdir>
git clone https://github.com/circt/circt.git
cd circt
git checkout firtool-1.72.0
git submodule init
git submodule update
```

#### LLVM/MLIR installation

Set `MLIR_DIR` environment variable to the directory with MLIR CMake files:

```shell
export MLIR_DIR=<workdir>/circt/llvm/build/lib/cmake/mlir/
```

Type the following commands:

```shell
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

#### CIRCT installation

Set `CIRCT_DIR` environment variable to the directory with CIRCT CMake files:

```shell
export CIRCT_DIR=<workdir>/circt/build/lib/cmake/circt/
```

Type the following commands:

```shell
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

### Clone project repository and set environment variables

```shell
cd <workdir>
git clone --recursive https://gitlab.ispras.ru/mvg/utopia-eda.git
cd utopia-eda
export UTOPIA_HOME=<workdir>/utopia-eda
export Yosys_ROOT=<workdir>/yosys
```

Please keep `UTOPIA_HOME` and `Yosys_ROOT` variables and the values in your system permanently.

### Project building

```shell
cd utopia-eda
cmake -S . -B build -G Ninja
cmake --build build
```

or simply run the following script:

```shell
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

## Running Utopia EDA

Running `umain` without arguments takes you to the TCL shell. Available commands
are listed in the `doc/CLI.md`, or can be printed using the `help`
command in the shell:

```shell
./build/src/umain

help
```

**P.S. It is recommended to use rlwrap to display the interactive mode correctly:**

```shell
rlwrap ./build/src/umain
```

To pass a TCL script with arguments, do the following:

```shell
./build/src/umain -s <script-path> <arg1> <agr2> ... 
```

To execute TCL script obtained from terminal, do the following:

```shell
./build/src/umain -e "command1;command2;..." 
```

Use the ``-i`` flag to switch to interactive mode after script execution:

```shell
./build/src/umain -ie "command1;command2;..." 
```

Example:

```shell
./build/src/umain -s scripts/synth_graphml.tcl \
test/data/openabcd-subset/graphml/sasc_orig.bench.graphml \
test/data/gate/techmapper/sky130_fd_sc_hd__ff_100C_1v65.lib
```

## Testing

### All tests running

```shell
rm -rf $UTOPIA_HOME/output
$UTOPIA_HOME/build/test/utest
```

or

```shell
./run-tests.sh
```

### Specific tests running

```shell
./build/test/utest --gtest_filter=<test-pattern>
```

or

```shell
./filter-tests.sh <test-pattern>
```

or set the `GTEST_FILTER` environment variable to filter some tests out:

```shell
export GTEST_FILTER="-ReedMuller*:\
BiDecompositionTest*"

./build/test/utest
```

Test pattern accepts ```*``` and ```?``` wildcards.

### List available tests

```shell
./build/test/utest --gtest_list_tests
```

### Code coverage

To evaluate code coverage is reached by tests, use `lcov.sh` script.
