mkdir build
cd build

echo building in %cd%
cmake -G"Visual Studio 16 2019" -A x64 -DCMAKE_CXX_STANDARD=20 -DWITH_CTEST=Off -DCMAKE_INSTALL_PREFIX=%INSTALL_PREFIX% -DPYTHON_EXECUTABLE=%CONDA_PREFIX%\python .. || echo ERROR && exit /b 1

:: Show cmake settings
cmake -B . -S .. -LA

:: Benchmarks
cmake --build . --target all-benchmarks --config Release -- /m:%NUMBER_OF_PROCESSORS% || echo ERROR && exit /b 1

:: C++ tests
cmake --build . --target all-tests --config Release -- /m:%NUMBER_OF_PROCESSORS% || echo ERROR && exit /b 1
bin\Release\scipp-common-test.exe || echo ERROR && exit /b 1
bin\Release\scipp-core-test.exe || echo ERROR && exit /b 1
bin\Release\scipp-variable-test.exe || echo ERROR && exit /b 1
bin\Release\scipp-dataset-test.exe || echo ERROR && exit /b 1
bin\Release\scipp-units-test.exe || echo ERROR && exit /b 1

:: Build and install Python library
cmake --build . --target install --config Release -- /m:%NUMBER_OF_PROCESSORS% || echo ERROR && exit /b 1
