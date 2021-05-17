if "%INSTALL_PREFIX%" == "" set INSTALL_PREFIX=%cd%\scipp_install & call tools\make_and_install.bat

move %INSTALL_PREFIX%\scipp %CONDA_PREFIX%\lib\
move %INSTALL_PREFIX%\bin\scipp-*.dll %CONDA_PREFIX%\bin\
move %INSTALL_PREFIX%\bin\units-shared.dll %CONDA_PREFIX%\bin\
move %INSTALL_PREFIX%\Lib\scipp-*.lib %CONDA_PREFIX%\Lib\
move %INSTALL_PREFIX%\Lib\units-shared.lib %CONDA_PREFIX%\Lib\
move %INSTALL_PREFIX%\Lib\cmake\scipp %CONDA_PREFIX%\Lib\cmake\
move %INSTALL_PREFIX%\include\scipp %CONDA_PREFIX%\include\
move %INSTALL_PREFIX%\include\scipp-* %CONDA_PREFIX%\include\
move %INSTALL_PREFIX%\include\Eigen %CONDA_PREFIX%\include\
move %INSTALL_PREFIX%\include\units %CONDA_PREFIX%\include\
