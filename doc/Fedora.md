[//]: <> (SPDX-License-Identifier: Apache-2.0)

# Utopia EDA Installation Guide (for Fedora Linux OS)

This guide describes main steps and rules that should be done and followed for building Utopia EDA on RPM-like distro.

All the steps were made on Fedora 39.

## Installing basic packages

First of all, install next packages:

```
sudo dnf groupinstall "Development Tools" && sudo dnf install autoconf bison \
    openssl-devel libtool lld make ninja-build python python3-pip zlib zlib-devel \
    flex readline-devel gawk tcl-devel libffi-devel git graphviz g++ cmake \
    pkgconf-pkg-config boost-system boost-python3-devel boost-filesystem python-xdot \
    clang clang-tools-extra lpsolve lpsolve-devel
```

## Installing libraries

### C++ CTemplate Installation

```
cd <workdir>
git clone https://github.com/OlafvdSpek/ctemplate.git
cd ctemplate
./autogen.sh
./configure --prefix=/usr
make -j $(nproc)
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
./configure --enable-obj --enable-shared --prefix=/usr
make -j $(nproc)
sudo make install
```

As `CUDD` is installed not in default directory by using
`--prefix` option of configure script, then building `Utopia EDA`
will require environment variable `CUDD_DIR` that contains the path
to the `CUDD` actual installation directory. By default, cudd would be installed to
`/usr/lib64`, but it also can be installed in `/usr/lib`. Please, check both folders.
```
ls /usr/lib | grep cudd
ls /usr/lib64 | grep cudd
```
And set correct folder name as `CUDD_DIR`:
```
export CUDD_DIR=<correct_dir>
```

### STACCATO Installation

The `<path_to_cudd_dir>` refers to the path to the CUDD sources directory.

```
cd <workdir>
git clone https://github.com/ispras/staccato
cd staccato
make BUILD_TYPE=shared CUDD_INCLUDE=/usr/include SM="-DDISABLE_SM" -j $(nproc)
sudo make install
```

### Configuring with `Yosys`

1. Get `Yosys` version 0.36 source code from the [^yosys] into `<yosys-dir>`.
2. Edit `<yosys-dir>/Makefile`
    - set `ENABLE_LIBYOSYS` to 1
3. Build and install `Yosys` as described in the `<yosys-dir>/README.md`, 
    except packages installation, as they were installed earlier

[^yosys]: https://github.com/YosysHQ/yosys/archive/refs/tags/yosys-0.36.tar.gz  

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
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
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

### Before Building
After doing all this steps, reboot your pc:
```
reboot
```

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
