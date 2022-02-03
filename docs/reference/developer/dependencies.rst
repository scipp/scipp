Dependencies
============

Places where dependencies are specified:

  - Conda package build (`conda/meta.yml <https://github.com/scipp/scipp/blob/main/conda/meta.yaml>`_)
  - Conan file (`conanfile.txt <https://github.com/scipp/scipp/blob/main/lib/conanfile.txt>`_)

Conda packages
##############

Required conda packages to reproduce the build environment are fully specified in `conda/meta.yaml <https://github.com/scipp/scipp/blob/main/conda/meta.yaml>`_
These include runtime dependencies for scipp.
  
Conan
#####

Required conan dependencies are fully specified in `conanfile.txt <https://github.com/scipp/scipp/blob/main/lib/conanfile.txt>`_
Note that these are brought into the project at cmake-time for the purposes of building and testing scipp.
