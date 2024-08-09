# Utopia EDA

Utopia is an EDA (Electronic Design Autiomation) framework for logic synthesis of digital hardware designs.

## Licensing and distribution

Utopia is distributed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

## Coding style

See `doc/CodeStyle.md` for more details about our coding convention.

## General notes

See `doc/Notes.md` if you're not familiar with program building/installing on Linux.

## System requirements

The recommended operating system for Utopia is Ubuntu 20.04. The package names
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
* `gtkwave`
* `iverilog`
* `liblpsolve55-dev`
* `libssl-dev`
* `libtool`
* `lld`
* `make`
* `ninja-build`
* `pkg-config`
* `python3`
* `rlwrap`
* `zlib1g`
* `zlib1g-dev`

To install them, do the following:

```console
sudo apt install autoconf bison build-essential clang clang-tidy cmake doxygen \
     flex g++ gcc git graphviz gtkwave iverilog libfmt-dev liblpsolve55-dev \
     libssl-dev libtool lld make ninja-build pkg-config python3 rlwrap zlib1g \
     zlib1g-dev
```

If you are working on Fedora Linux OS, see `doc/Fedora.md`.

### CMake installation

To build the appropriate version of CMake from sources, do the following:

```console
sudo apt install tar wget
cd <workdir>
wget https://cmake.org/files/v3.28/cmake-3.28.1.tar.gz
tar xzf cmake-3.28.1.tar.gz
rm -rf cmake-3.28.1.tar.gz
cd cmake-3.28.1
./bootstrap
make
sudo make install
```

If you prefer to install CMake as package, please follow this [guide](https://apt.kitware.com).

### C++ CTemplate installation

To build C++ CTemplate library from sources, do the following:

```console
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

### CUDD installation

To build CUDD library from sources, do the following:

```console
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

```console
cd <workdir>
git clone https://github.com/ispras/staccato
cd staccato
make BUILD_TYPE=shared CUDD_INCLUDE=<cudd-path> SM="-DDISABLE_SM"
sudo make install
```

### Yosys installation

1. Get `Yosys` version 0.36 source code from [repo](https://github.com/YosysHQ/yosys/tree/yosys-0.36) into `<yosys-dir>`.

   ```console
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

```console
cd <workdir>
git clone https://github.com/circt/circt.git
cd circt
git checkout firtool-1.72.0
git submodule init
git submodule update
```

#### LLVM/MLIR installation

Set `MLIR_DIR` environment variable to the directory with MLIR CMake files:

```console
export MLIR_DIR=<workdir>/circt/llvm/build/lib/cmake/mlir/
```

Type the following commands:

```console
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

```console
export CIRCT_DIR=<workdir>/circt/build/lib/cmake/circt/
```

Type the following commands:

```console
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

## Working in command line

### Clone project repository and set environment variables

```console
cd <workdir>
git clone --recursive https://gitlab.ispras.ru/mvg/utopia-eda.git
cd utopia-eda
export UTOPIA_HOME=<workdir>/utopia-eda
export Yosys_ROOT=<workdir>/yosys
```

Please keep `UTOPIA_HOME` and `Yosys_ROOT` variables and the values in your system permanently.

### Project building

```console
cd utopia-eda
cmake -S . -B build -G Ninja
cmake --build build
```

or simply run the following script:

```console
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

Running `umain` without arguments takes you to the TCL shell. Available commands
are listed in the `doc/CLI.md`, or can be printed using the `help`
command in the shell:

```console
./build/src/umain

help
```

**P.S. It is recommended to use rlwrap to display the interactive mode correctly:**

```console
rlwrap ./build/src/umain
```

To pass a TCL script with arguments, do the following:

```console
./build/src/umain -s <script-path> <arg1> <agr2> ... 
```

To execute TCL script obtained from terminal, do the following:

```console
./build/src/umain -e "command1;command2;..." 
```

Use the ``-i`` flag to switch to interactive mode after script execution:

```console
./build/src/umain -ie "command1;command2;..." 
```

Example:

```console
./build/src/umain -s scripts/synth_graphml.tcl \
test/data/openabcd-subset/graphml/sasc_orig.bench.graphml \
test/data/gate/techmapper/sky130_fd_sc_hd__ff_100C_1v65.lib
```

### Tests running

#### All tests running

```console
rm -rf $UTOPIA_HOME/output
$UTOPIA_HOME/build/test/utest
```

or

```console
./run-tests.sh
```

#### Specific tests running

```console
./build/test/utest --gtest_filter=<test-pattern>
```

or

```console
./filter-tests.sh <test-pattern>
```

or set the `GTEST_FILTER` environment variable, for example, as follows:

```console
export GTEST_FILTER="-ReedMuller*:\
BiDecompositionTest*"

./build/test/utest
```

Test pattern accepts ```*``` and ```?``` wildcards.

#### List available tests

```console
./build/test/utest --gtest_list_tests
```

#### Code coverage

To evaluate code coverage is reached by tests, use `lcov.sh` script.
