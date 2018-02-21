# Done

A functor `Alg` with `apply` method defines an algorithm.
It is invoked with the help of a templated `call` wrapper.
Supported features are:

- Algorithms with single input workspace.
- If the constructor has arguments, all arguments but the first workspace are passed to the constructor.
- If the constructor has no arguments, all arguments but the first are passed to `Alg::apply` as second and subsequent arguments.
- The `call` wrapper takes the workspace by-value. In-place operation can be achieved by moving the input workspace.
- If `Alg::apply` accepts meta data components of the workspace it is invoked with that meta data as first argument.
- If `Alg::apply` accepts data items of the workspace it is invoked for all data items in the workspace as first argument.
- Mutating the type of the items in a workspace is supported, e.g., for `Rebin` converting `EventList` to `Histogram`.
- The workspace type can depend on more than just the data type, e.g., the instrument type (`SpectrumInfo`, `QInfo`, ...).
  Algorithm operating on other meta data or data items are unaffected and work without knowledge of that aspect of the workspace type.

# Potential problems

- Functor signatures that are not supported by call wrappers result in template error messages that would probably be too obscure for many developers, unless documented properly.

# To do

- Should we support history in C++?
  If we have the ADS and property system only in Python, we do not have access to the string values of inputs.
  Could probably be solved in several ways, but it is not clear whether that is the right way to go.
- Support logging by having an optional logger argument in the functor constructor?
- Automatically do threaded execution if `Functor::apply` is const.
