# Configuring the build using a CIRCT binary distribution

* Download a distribution from the [GitHub
  repository](https://github.com/llvm/circt/releases/tag/firtool-1.61.0)
  * For the build the full distribution is required, e.g.
    [this](https://github.com/llvm/circt/releases/download/firtool-1.61.0/circt-full-shared-linux-x64.tar.gz)
  * While the instructions are presumably relevant for any binary release,
    the only currently supported CIRCT version is 1.61.0
* Unpack the downloaded archive to a location of your choice
  * e.g. `tar xzf circt-full-shared-linux-x64-tar.gz -C <CIRCT_LIBDIR>`
* Edit the installed CMake configuration files
  * Somehow the CIRCT build scripts are a bit inconsistent resulting in a
    broken configuration
  * Open the file
    `<CIRCT_LIBDIR>/firtool-X.YY.Z/lib/cmake/mlir/MLIRTargets.cmake` with a
    text editor of your chooice
  * Search for the pattern `_NOT_FOUND_MESSAGE_targets` near the end of the
    file
  * Remove all `"CIRCT*"` entries in the `foreach` statement found right
    after the first pattern match
    * e.g. for version 1.61.1 the `foreach` statement is on the line 2799
  * Save the file
* Configure the build to use the binary distribution
  * Unset the environment variables `CIRCT_DIR` and `MLIR_DIR` if they've
    been set previously
    * e.g. `unset MLIR_DIR CIRCT_DIR`
  * Pass the option `-DCMAKE_PREFIX_PATH=<CIRCT_LIBDIR>/firtool-X.YY.Z` in
    addition to the configuration options of your choice
    * e.g. `cmake -S $UTOPIA_HOME -B $UTOPIA_HOME/build -G Ninja -DCMAKE_PREFIX_PATH=<CIRCT_LIBDIR/firtool-1.61.0`
