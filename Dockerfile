FROM gitlab.ispras.ru:4567/mvg/utopia-eda:base

# Build & configure Utopia EDA
WORKDIR /workdir
# RUN git clone --recursive https://github.com/hqdem/hlimds.git
# RUN git checkout lab1
COPY . ./utopia-eda
ENV UTOPIA_HOME="/workdir/utopia-eda"
WORKDIR ${UTOPIA_HOME}
RUN cmake -S ${UTOPIA_HOME} -B build -G Ninja
RUN cmake --build build

# Set up the interpreter
WORKDIR /workdir
CMD ["/bin/bash"]
