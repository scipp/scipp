#!/bin/sh

# Check that the source conforms to the clang-format style

# Find clang-format executable
CLANG_FORMAT_EXE=clang-format-5.0
if [ -z "$(which $CLANG_FORMAT_EXE)" ]; then
  echo "Cannot find ${CLANG_FORMAT_EXE} executable"
  exit 1
else
  ${CLANG_FORMAT_EXE} --version
fi

# Perform formatting
find core scippy test units -type f -name '*.h' -o -name '*.cpp' | xargs -I{} ${CLANG_FORMAT_EXE} -i -style=file {};
DIRTY=$(git ls-files --modified);

if [ -z "${DIRTY}" ]; then
    echo "Clang format [ OK ]";
    exit 0;
else
  echo "Clang format FAILED on the following files:";
  echo "${DIRTY}" | tr ' ' '\n';
  exit 1;
fi
