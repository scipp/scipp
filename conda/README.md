# Conda packaging

Actual conda releases of scipp are built by the
[conda-forge feedstock](https://github.com/conda-forge/scipp-feedstock).
The recipe in this directory (`recipe.yaml`, rattler-build v1 format) mirrors
the feedstock recipe and exists so CI can continuously test the conda packaging
code path — catching breakage (new dependencies, CMake changes, pin conflicts
with conda-forge's global pinning) *before* a release reaches the feedstock.
Keep the dependencies here in sync with the feedstock recipe.

## Building locally

The build runs through the pixi `package` environment, which provides
`rattler-build` and `conda-forge-pinning` (the same global pins conda-forge
uses). A `dynamic_var.yaml` in the repository root must select one python
version (CI generates it; see `.github/workflows/conda.yml`), e.g.:

```yaml
python:
- 3.11.* *_cpython
```

Then, from the root of the repository:

```sh
pixi run -e package conda-package <target>
```

where `<target>` is one of the variant files in `conda/variants/`
(`linux_64`, `linux_arm64`, `osx_64`, `osx_arm64`, `win_64`) matching the
machine you are building on. Packages are written to `conda/package/`.

## Version numbering

Conda packages have a version number and build number.
The version number is taken from the last Git tag.
The build number is the number of commits since the last tag.
Both are computed by the `conda-package` pixi task (mirroring conda-build's
`GIT_DESCRIBE_TAG`/`GIT_DESCRIBE_NUMBER`), so the full git history and tags
must be available (`fetch-depth: 0` in CI).
