The "What's New" pages require a specific version to be generated.
Therefore, we only generate them when a release is made, and the link to those pages subsequently.

## Required steps:

### For the release

- Add new `ipynb` for a release.
- Make sure it does *not* have `"nbsphinx": { "execute": "never" }` set in the notebook metadata.
- Add an *absolute* link to `whats-new.rst`, e.g.,
  ```
  `scipp-0.7 <https://scipp.github.io/release/0.7.0/about/whats-new/whats-new-0.7.0.html>`_
  ```
  Note that for the release a relative link would also work, but for building docs for subsequent dev builds or releases we need the link to be absolute, since these pages will not be built.
- Tag the release.
  This will build the page.

### After the release

- Add `"nbsphinx": { "execute": "never" }` to the notebook metadata.
