# CMake installation

## Package installation

If you prefer to install CMake as package, please follow this [guide](https://apt.kitware.com).

## Building from sources

To build CMake 3.28.1 from sources, do the following:

```shell
sudo apt install tar wget
cd <workdir>
wget https://cmake.org/files/v3.28/cmake-3.28.1.tar.gz
tar xzf cmake-3.28.1.tar.gz
rm -rf cmake-3.28.1.tar.gz
cd cmake-3.28.1
./bootstrap
make -j$(nproc)
sudo make install
```
