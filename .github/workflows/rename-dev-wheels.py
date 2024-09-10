import glob
import os

for filename in glob.glob('dist/*whl'):
    # We remove the version number, dev number, and Git hash to ensure we have a
    # predictable URL of the uploaded release asset that downstream projects can use.
    # We have to use a dummy version number as this is required by
    # pip (https://github.com/pypa/pip/issues/12063) and
    # uv (https://github.com/astral-sh/uv/issues/5478)
    pkg, _, remainder = filename.split('-', 2)
    target = f'{pkg}-9999.99.99-{remainder}'
    print(f'Renaming wheel {filename} to {target}')
    os.rename(filename, target)
