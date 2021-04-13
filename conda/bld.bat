
if "%INSTALL_TARGET%" == "" call tools\make_and_install.bat & set INSTALL_TARGET=%CONDA_PREFIX%

dir /s/b %INSTALL_TARGET%

move %INSTALL_TARGET%\scipp %CONDA_PREFIX%\lib\
move %INSTALL_TARGET%\bin\scipp-*.dll %CONDA_PREFIX%\bin\
move %INSTALL_TARGET%\bin\units-shared.dll %CONDA_PREFIX%\bin\
move %INSTALL_TARGET%\Lib\scipp-*.lib %CONDA_PREFIX%\Lib\
move %INSTALL_TARGET%\Lib\units-shared.lib %CONDA_PREFIX%\Lib\
move %INSTALL_TARGET%\Lib\cmake\scipp %CONDA_PREFIX%\Lib\cmake\
move %INSTALL_TARGET%\include\scipp* %CONDA_PREFIX%\include\
move %INSTALL_TARGET%\include\Eigen %CONDA_PREFIX%\include\
move %INSTALL_TARGET%\include\units %CONDA_PREFIX%\include\
