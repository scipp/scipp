import glob
import os

for filename in glob.glob('dist/*whl'):
    # We remove the version number, dev number, and Git hash to ensure we have a
    # predictable URL of the uploaded release asset that downstream projects can use.
    pkg, _, remainder = filename.split('-', 2)
    target = f'{pkg}-nightly-{remainder}'
    print(f'Renaming wheel {filename} to {target}')
    os.rename(filename, target)
