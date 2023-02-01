# Stub generator

This package generates a stub file for classes defined in C++ in Scipp.
It is specialized to Scipp and will not work with other projects.

## Usage

Simply call the package:

```sh
python -m stubgen
```

or use tox:

```shell
tox -e stubgen
```

This will build a new stub file by importing and inspecting scipp.
So you need to make sure that it has built and is importable.
In tox, this can be achieved by calling `tox -e lib && tox -e editable && mamba develop src`.

The output is written to the correct place in the source tree by default.
This can be changed with a command line argument.

## Configuring and fixing issues

There are some configuration options in `config.py`.
If the generator breaks because of changes to C++, it may be enough to adjust those options.

See also the node transformers in `transformer.py` for details of how the code gets adjusted for various edge cases and oddities coming from Pybind11.

## How it works

For Python functions, the generator simply inspects the function's source code and parses it into an AST.
For functions bound via Pybind11, it inspects docstrings to determine the function signature.
This relies on Pybind11 to generate a docstring of the correct format.
For regular functions, the docstring should be along the lines of (see `parse.py`)
```
func_name(arg1: type1, arg2: type2) -> return_type

Description
```
For overloaded functions, it should be
```
func_name(*args, **kwargs)
Overloaded function.

1. func_name(arg1: type11, arg2: type12) -> return_type1

Description

2. func_name(arg1: type21, arg2: type22) -> return_type2

Description
```

It is possible to customize the generated stubs by including the desired signature in the docstring in the bindings.
