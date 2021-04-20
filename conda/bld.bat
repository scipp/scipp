
if "%INSTALL_PREFIX%" == "" set INSTALL_PREFIX=%cd% & call tools\make_and_install.bat

robocopy %INSTALL_PREFIX%\scipp %CONDA_PREFIX%\lib\scipp /e /move
robocopy %INSTALL_PREFIX%\bin %CONDA_PREFIX%\bin\ scipp-*.dll /e /move
robocopy %INSTALL_PREFIX%\bin %CONDA_PREFIX%\bin\ units-shared.dll /e /move
robocopy %INSTALL_PREFIX%\Lib %CONDA_PREFIX%\Lib\ scipp-*.lib /e /move
robocopy %INSTALL_PREFIX%\Lib %CONDA_PREFIX%\Lib\ units-shared.lib /e /move
robocopy %INSTALL_PREFIX%\Lib\cmake\scipp %CONDA_PREFIX%\Lib\cmake\scipp /e /move
robocopy %INSTALL_PREFIX%\include %CONDA_PREFIX%\include\ scipp*.h /e /move
robocopy %INSTALL_PREFIX%\include\scipp %CONDA_PREFIX%\include\scipp /e /move
robocopy %INSTALL_PREFIX%\include\Eigen %CONDA_PREFIX%\include\Eigen /e /move
robocopy %INSTALL_PREFIX%\include\units %CONDA_PREFIX%\include\units /e /move
