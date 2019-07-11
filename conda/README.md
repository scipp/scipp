# Conda packaging

To package you will need `conda-build`, to publish you will need `anaconda`.
Both can be installed using `conda install conda-build anaconda`.
These should be installed in your root/base environment.

To build, from the root of the repsository run `conda-build ./conda`.
A list of packages that are created will be shown at the end of the build process.
These can be installed locally or uploaded to Anaconda Cloud.
