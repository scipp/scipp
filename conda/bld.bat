
if "%INSTALL_PREFIX%" == "" set INSTALL_PREFIX=%cd% & call tools\make_and_install.bat

dir /s/b %INSTALL_PREFIX%

if not exist "%CONDA_PREFIX%\lib\scipp" mkdir %CONDA_PREFIX%\lib\scipp
move %INSTALL_PREFIX%\scipp\* %CONDA_PREFIX%\lib\scipp\
move %INSTALL_PREFIX%\bin\scipp-*.dll %CONDA_PREFIX%\bin\
move %INSTALL_PREFIX%\bin\units-shared.dll %CONDA_PREFIX%\bin\
move %INSTALL_PREFIX%\Lib\scipp-*.lib %CONDA_PREFIX%\Lib\
move %INSTALL_PREFIX%\Lib\units-shared.lib %CONDA_PREFIX%\Lib\
if not exist "%CONDA_PREFIX%\Lib\cmake\scipp" mkdir %CONDA_PREFIX%\Lib\cmake\scipp
move %INSTALL_PREFIX%\Lib\cmake\scipp\* %CONDA_PREFIX%\Lib\cmake\scipp\
move %INSTALL_PREFIX%\include\scipp*.h %CONDA_PREFIX%\include\
if not exist "%CONDA_PREFIX%\include\scipp" mkdir %CONDA_PREFIX%\include\scipp
move %INSTALL_PREFIX%\include\scipp\* %CONDA_PREFIX%\include\scipp\
if not exist "%CONDA_PREFIX%\include\Eigen" mkdir %CONDA_PREFIX%\include\Eigen
move %INSTALL_PREFIX%\include\Eigen\* %CONDA_PREFIX%\include\Eigen\
if not exist "%CONDA_PREFIX%\include\units" mkdir %CONDA_PREFIX%\include\units
move %INSTALL_PREFIX%\include\units\* %CONDA_PREFIX%\include\units\
