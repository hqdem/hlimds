[//]: <> (SPDX-License-Identifier: Apache-2.0)

# Utopia EDA

Utopia is an EDA for logic synthesis of digital hardware designs.

## Licensing and Distribution

Utopia is distributed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

## Coding Style

See `doc\CODE_STYLE.md` for more details about our coding convention.

## General Notes

See `doc\NOTES.md` if you're not familiar with program building/installation on Linux.

## System Requirements

The recommended operating system for Utopia is Ubuntu 20.04. The package names
below are specific to this operating system:

* `autoconf`
* `bison`
* `clang`
* `clang-tidy`
* `cmake`
* `doxygen`
* `flex`
* `g++`
* `gcc`
* `graphviz`
* `iverilog`
* `libfmt-dev`
* `liblpsolve55-dev`
* `libtool`
* `lld`
* `make`
* `ninja-build`
* `python`
* `python3-pip`
* `zlib1g`
* `zlib1g-dev`

To install them, do the following:
```
sudo apt install autoconf bison clang clang-tidy cmake doxygen flex g++ gcc \
    graphviz iverilog liblpsolve55-dev libtool lld make ninja-build python \
    python3-pip zlib1g zlib1g-dev
```
Several Python packages should be installed too. Do the following:
```
pip install liberty-parser
```

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
./configure --enable-obj
make
sudo make install
```

If you want to install `CUDD` not in default directory by using
`--prefix` option of configure script, then building `Utopia EDA`
will require environment variable `CUDD_DIR` that contains the path
to the `CUDD` actual installation directory.

### Configuring with `Yosys`

1. Get `Yosys` source code from the [^yosys] into `<yosys-dir>`
2. Make sure your system meets the requirements listed in `<yosys-dir>/README.md`
3. Edit `<yosys-dir>/Makefile`
    - set `ENABLE_LIBYOSYS` to 1
4. Build and install `Yosys` as described in the `<yosys-dir>/README.md`
5. Configure `Utopia` to find `Yosys`
    - add `-DYosys_ROOT=<yosys-dir>` to the `cmake` invocation
    - e.g. `cmake -S <utopia-source-dir> -B <utopia-build-dir> -DYosys_ROOT=<yosys-dir> -G Ninja`

[^yosys]: https://github.com/YosysHQ/yosys


## Working in Command Line

### Clone Project Repository and Set Environment Variable

```
cd <workdir>
git clone --recursive https://gitlab.ispras.ru/mvg/utopia-eda.git
cd utopia-eda
export UTOPIA_HOME=<workdir>/utopia-eda
```
Please keep `UTOPIA_HOME` variable and its value in your system permanently.
### Project Building 

```
cd utopia-eda
cmake -S . -B build -DYosys_ROOT=<yosys-dir> -G Ninja
cmake --build build
```
or simply run the following script:
```
./build.sh
```
If you've modified some of the project files, you can use `rebuild.sh` script
for incremental build.

During the project building, Doxygen (if installed) generates the documentation
in HTML and LaTeX formats. The generated documentation is stored
at the `build/doc` directory.

### Running Utopia EDA

```
rm -rf $UTOPIA_HOME/output
./build/src/umain rtl <file(s)> <options>
```
To list the Utopia EDA options, do the following:
```
./build/src/umain --help-all
```

#### Run Verilog-to-FIRRTL translator

```
./build/src/umain to_firrtl <file> --top <module-name>
```
When selecting this option, you must specify the name of the top module
(`module-name`) in the Verilog file and the path to the file itself (`file`).
The results of the translation are: `*.fir` description and a file with
debugging information. These files will be generated in the same directory
as the input Verilog file.


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

#### List Available Tests

```
./build/test/utest --gtest_list_tests
```
