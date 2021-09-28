# Utopia EDA

Utopia is an open-source HLS-based EDA for digital hardware.

## System Requirements

* `gcc`
* `make`
* `cmake`
* `flex`
* `bison`

## Working in Command Line

## Building Project

```
cd src
mkdir build
cd build
cmake ..
make
```
## Running Unit Tests

TODO

## Working in Visual Studio Code

### Installing VS Studio w/ C/C++ and CMake Extensions
* Download the VS Studio package
  * Go to https://code.visualstudio.com/docs/?dv=linux64_deb
  * Wait until the package is downloaded
* Install the VS Studio package
  ```
  sudo sudo apt install -f ~/Downloads/code_1.60.2-1632313585_amd64.deb
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

### Running Unit Tests

TODO
