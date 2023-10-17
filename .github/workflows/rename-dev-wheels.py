import glob
import os

for filename in glob.glob('dist/*whl'):
    base, remainder = filename.split('dev')
    end = remainder[1].split('cp', 1)[1]
    # We remove the Git hash and set the dev version to 0, to ensure we have a
    # predictable URL of the uploaded release asset that downstream projects can use.
    target = f"{base}dev0-cp{end}"
    os.rename(filename, target)
