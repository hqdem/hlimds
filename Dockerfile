FROM isprasmvg/ubuntu:base

RUN apt-get update && apt install -y autoconf bison clang clang-tidy cmake flex g++ gcc \
    iverilog liblpsolve55-dev libtool libxerces-c3.2 libxerces-c-dev lld \
    make ninja-build python3 python3-pip zlib1g zlib1g-dev
RUN pip install liberty-parser
RUN apt-get update && apt install -y lcov

WORKDIR /workdir
RUN git clone https://github.com/OlafvdSpek/ctemplate.git
WORKDIR /workdir/ctemplate
RUN ./autogen.sh
RUN ./configure --prefix=/usr
RUN make -j$(nproc)
RUN make install

WORKDIR /workdir
RUN git clone https://github.com/ivmai/cudd
WORKDIR /workdir/cudd
RUN touch aclocal.m4 Makefile.am Makefile.in configure
RUN ./configure --enable-obj --enable-shared
RUN make -j$(nproc)
RUN make install

WORKDIR /workdir
RUN git clone https://github.com/YosysHQ/yosys.git
ENV old_line="ENABLE_LIBYOSYS := 0"
ENV new_line="ENABLE_LIBYOSYS := 1"
ENV file_path="/workdir/yosys/Makefile"
RUN sed -i "s|$old_line|$new_line|" "$file_path"
WORKDIR /workdir/yosys
RUN make -j$(nproc)
RUN make install

WORKDIR /workdir
RUN git clone --recursive https://github.com/circt/circt.git
WORKDIR /workdir/circt
RUN git checkout 2d822ea
RUN git submodule init
RUN git submodule update
ENV MLIR_DIR=/workdir/circt/llvm/build/lib/cmake/mlir/
RUN mkdir llvm/build
WORKDIR /workdir/circt/llvm/build
RUN cmake -G Ninja ../llvm \
    -DLLVM_ENABLE_PROJECTS="mlir;clang;clang-tools-extra;lld" \
    -DLLVM_TARGETS_TO_BUILD="X86" \
    -DCMAKE_BUILD_TYPE="Release" \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DLLVM_ENABLE_LLD=ON \
    -DLLVM_PARALLEL_LINK_JOBS=$(nproc) \
    -DLLVM_PARALLEL_COMPILE_JOBS=$(nproc)
RUN ninja

ENV PATH=/workdir/circt/build/bin:/workdir/circt/llvm/build/bin:$PATH
ENV CIRCT_DIR=/workdir/circt/
WORKDIR /workdir/circt
RUN mkdir build
WORKDIR /workdir/circt/build
RUN cmake -G Ninja .. \
    -DCMAKE_BUILD_TYPE="Release" \
    -DLLVM_ENABLE_ASSERTIONS=ON \
    -DMLIR_DIR=$PWD/../llvm/build/lib/cmake/mlir \
    -DLLVM_DIR=$PWD/../llvm/build/lib/cmake/llvm \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DVERILATOR_DISABLE=ON \
    -DIVERILOG_DISABLE=ON
RUN ninja

WORKDIR /workdir
CMD ["/bin/bash"]
