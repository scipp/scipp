mkdir build

cd build

cmake -G"Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX=%CONDA_PREFIX% -DPYTHON_EXECUTABLE=%CONDA_PREFIX%\python -DWITH_CXX_TEST=OFF ..

cmake --build . --target install --config Release

move %CONDA_PREFIX%\scipp %CONDA_PREFIX%\lib\