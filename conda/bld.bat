mkdir build
cd build

echo building in %cd%
cmake -G"Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX=%CONDA_PREFIX% -DPYTHON_EXECUTABLE=%CONDA_PREFIX%\python -DWITH_CXX_TEST=OFF ..

:: Show cmake settings
cmake -B . -S .. -LA

:: C++ tests
cmake --build . --target all-tests --config Release || echo ERROR && exit /b
core\test\Release\scipp-core-test.exe || echo ERROR && exit /b
neutron\test\Release\scipp-neutron-test.exe || echo ERROR && exit /b
units\test\Release\scipp-units-test.exe || echo ERROR && exit /b

::  Build, install and move scipp Python library to site packages location
cmake --build . --target install --config Release || echo ERROR && exit /b
move %CONDA_PREFIX%\scipp %CONDA_PREFIX%\lib\
