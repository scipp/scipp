
if "%INSTALL_PREFIX%" == "" set INSTALL_PREFIX=%cd% & call tools\make_and_install.bat

if not exist "%CONDA_PREFIX%\lib\scipp" mkdir %CONDA_PREFIX%\lib\scipp
robocopy %INSTALL_PREFIX%\scipp %CONDA_PREFIX%\lib\scipp\ /e /move
robocopy %INSTALL_PREFIX%\bin %CONDA_PREFIX%\bin\ scipp-*.dll /e /move
robocopy %INSTALL_PREFIX%\bin %CONDA_PREFIX%\bin\ units-shared.dll /e /move
robocopy %INSTALL_PREFIX%\Lib %CONDA_PREFIX%\Lib\ scipp-*.lib /e /move
robocopy %INSTALL_PREFIX%\Lib %CONDA_PREFIX%\Lib\ units-shared.lib /e /move
if not exist "%CONDA_PREFIX%\Lib\cmake\scipp" mkdir %CONDA_PREFIX%\Lib\cmake\scipp
robocopy %INSTALL_PREFIX%\Lib\cmake\scipp %CONDA_PREFIX%\Lib\cmake\scipp\ /e /move
robocopy %INSTALL_PREFIX%\include %CONDA_PREFIX%\include\ scipp*.h /e /move
if not exist "%CONDA_PREFIX%\include\scipp" mkdir %CONDA_PREFIX%\include\scipp
robocopy %INSTALL_PREFIX%\include\scipp %CONDA_PREFIX%\include\scipp\ /e /move
if not exist "%CONDA_PREFIX%\include\Eigen" mkdir %CONDA_PREFIX%\include\Eigen
robocopy %INSTALL_PREFIX%\include\Eigen %CONDA_PREFIX%\include\Eigen\ /e /move
if not exist "%CONDA_PREFIX%\include\units" mkdir %CONDA_PREFIX%\include\units
robocopy %INSTALL_PREFIX%\include\units %CONDA_PREFIX%\include\units\ /e /move
