
if "%INSTALL_PREFIX%" == "" set INSTALL_PREFIX=%cd% & call tools\make_and_install.bat

dir /s/b %INSTALL_PREFIX%

robocopy %INSTALL_PREFIX%\scipp %CONDA_PREFIX%\lib\ /e /move
move %INSTALL_PREFIX%\bin\scipp-*.dll %CONDA_PREFIX%\bin\
move %INSTALL_PREFIX%\bin\units-shared.dll %CONDA_PREFIX%\bin\
move %INSTALL_PREFIX%\Lib\scipp-*.lib %CONDA_PREFIX%\Lib\
move %INSTALL_PREFIX%\Lib\units-shared.lib %CONDA_PREFIX%\Lib\
robocopy %INSTALL_PREFIX%\Lib\cmake\scipp %CONDA_PREFIX%\Lib\cmake\ /e /move
move %INSTALL_PREFIX%\include\scipp*.h %CONDA_PREFIX%\include\
robocopy %INSTALL_PREFIX%\include\scipp %CONDA_PREFIX%\include\ /e /move
robocopy %INSTALL_PREFIX%\include\Eigen %CONDA_PREFIX%\include\ /e /move
robocopy %INSTALL_PREFIX%\include\scipp %CONDA_PREFIX%\include\ /e /move
