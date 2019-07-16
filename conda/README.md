# Conda packaging

To package you will need `conda-build`, to publish you will need `anaconda`.
Both can be installed using `conda install conda-build anaconda`.
These should be installed in your root/base environment.

To build, from the root of the repository run `conda-build ./conda`.
A list of packages that are created will be shown at the end of the build process.
These can be installed locally or uploaded to Anaconda Cloud.

## Version numbering

Conda packages have a version number and build number.
The version number is taken from the last Git tag.
The build number is the number of commits since the last tag.
See [the Conda documentation](https://docs.conda.io/projects/conda-build/en/latest/user-guide/environment-variables.html#git-environment-variables) for more information..

## Development builds

Commits to the `master` branch trigger a build and deploy of the Conda packages.
These are published under the [`dev` label](https://anaconda.org/scipp/repo/files?type=all&label=dev).

## Release builds

Note that the release should be tagged in Git before creating the release package.

TODO: complete this after version numbers are discussed
