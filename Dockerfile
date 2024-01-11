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
CMD ["/bin/bash"]
