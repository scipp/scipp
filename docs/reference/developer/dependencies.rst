Dependencies
============

Places where dependencies are specified:

  - Conda package build (`conda/meta.yml <https://github.com/scipp/scipp/blob/main/conda/meta.yaml>`_)
  - Developer environment (`scipp-developer.yml <https://github.com/scipp/scipp/blob/main/scipp-developer.yml>`_)
  - Conan file (`condafile.txt <https://github.com/scipp/scipp/blob/main/conanfile.txt>`_) 

Conda packages
##############

Required conda packages to reproduce the build environment are fully specified in `scipp-developer.yml <https://github.com/scipp/scipp/blob/main/scipp-developer.yml>`_
These include runtime dependencies for scipp
  
Conan
#####

Required conan dependencies are fully specified in `condafile.txt <https://github.com/scipp/scipp/blob/main/conanfile.txt>`_
Note that these are brought into the project at cmake-time for the purposes of building and testing scipp.

