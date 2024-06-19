# General Notes

Several notes on project configuration, building etc.

## Environment variables

Several environment variables should be set during the Utopia building.
To keep the value of the environment variable in your system permanenly, add the
appropriate command to either `.profile` or `.bashrc` file. For example, to set
the `/usr` value to the `SOME_DIR` variable, the command should be as follows:

```console
export SOME_DIR=/usr
```

To make this variable-value pair active in your terminal session you need either
to reboot your OS or restart your session or run the following command
(it is supposed that the variable was initialized in `.profile`):

```console
source ~/.profile
```

To check if value is set, use `echo $SOME_DIR` command.

## Building speedup

To speed up building, several tools like `make` or `ninja` provide options
that aimed at running multiple jobs in parallel. For example, to run `make`
in 4 threads do the following:

```console
make -j4
```

It is recommended to use such options upon building the project dependencies,
because of significant reduction of the compilation time.

## Working in Visual Studio Code

### Installing VS Studio Code w/ C/C++ and CMake extensions

* Download the VS Studio package
  * Go to [VS Code page](https://code.visualstudio.com/docs/?dv=linux64_deb)
  * Wait until the package is downloaded
* Install the VS Studio package

  ```console
  sudo apt install -f ~/Downloads/code_1.60.2-1632313585_amd64.deb
  ```
  
* Install the C/C++ and CMake extensions
  * Start VS Code
  * Press the `Ctrl+Shift+x` key combination
    * Find and install the `C/C++` extension
    * Find and install the `CMake Tools` extension
  * Click on the `No Kit Selected` text in the status bar
    * Select the kit to use (for example, `GCC 9.3.0 x86_64-linux-gnu`)

### Opening/building project

* Clone project repository (see above)
* Click on the `File` and `Open Folder...` menu items
  * Select the `<UTOPIA_HOME>/src` directory (or `<UTOPIA_HOME>`)
  * Press the `I trust the authors` button
* Click on the `Build` text in the status bar
