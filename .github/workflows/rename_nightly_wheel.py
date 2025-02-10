import glob
import os

for filename in glob.glob('dist/*whl'):
    # Rename nightly wheels to the standard PyPI complaint X.X.X.dev0
    pkg, version, remainder = filename.split('-', 2)
    version = version.split('dev')[0] + 'dev0'
    target = f'{pkg}-{version}-{remainder}'
    print(f'Renaming wheel {filename} to {target}')
    os.rename(filename, target)
