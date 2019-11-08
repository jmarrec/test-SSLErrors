# Testbed for SSL error

cf: https://github.com/NREL/OpenStudioApplication/issues/40

```
git clone git@github.com:jmarrec/test-SSLErrors.git
cd test-SSLErrors

mkdir build && cd build
cmake -G Ninja -DEXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug ..
ninja
lldb Products/OpenStudioApp
```
