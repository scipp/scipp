
if "%INSTALL_PREFIX%" == "" set INSTALL_PREFIX=%cd% & call tools\make_and_install.bat

dir /s/b %INSTALL_PREFIX%

xcopy %INSTALL_PREFIX%\scipp %CONDA_PREFIX%\lib\ /s /e
xcopy %INSTALL_PREFIX%\bin\scipp-*.dll %CONDA_PREFIX%\bin\ /s /e
xcopy %INSTALL_PREFIX%\bin\units-shared.dll %CONDA_PREFIX%\bin\ /s /e
xcopy %INSTALL_PREFIX%\Lib\scipp-*.lib %CONDA_PREFIX%\Lib\ /s /e
xcopy %INSTALL_PREFIX%\Lib\units-shared.lib %CONDA_PREFIX%\Lib\ /s /e
xcopy %INSTALL_PREFIX%\Lib\cmake\scipp %CONDA_PREFIX%\Lib\cmake\ /s /e
xcopy %INSTALL_PREFIX%\include\scipp*.h %CONDA_PREFIX%\include\ /s /e
xcopy %INSTALL_PREFIX%\include\scipp %CONDA_PREFIX%\include\ /s /e
xcopy %INSTALL_PREFIX%\include\Eigen %CONDA_PREFIX%\include\ /s /e
xcopy %INSTALL_PREFIX%\include\units %CONDA_PREFIX%\include\ /s /e

dir /s/b %CONDA_PREFIX%
