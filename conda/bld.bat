
if "%INSTALL_TARGET%" == "" call tools\make_and_install.bat & set INSTALL_TARGET=%CONDA_PREFIX%

dir /s/b %INSTALL_TARGET%

move %INSTALL_TARGET%\scipp %CONDA_PREFIX%\lib\
