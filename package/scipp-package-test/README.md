Tiny dummy project for testing the cmake configuration and library setup of the scipp package.

Use as follows:

```
mkdir -p build && cd build
cmake -DCMAKE_PREFIX_PATH="$(scippinstall_dir)" ..
cmake --build ..
./test
```
