Key problems to solve:
- Link various information together, including instrument, logs, related workspaces, masking.
- How can we write generic algorithms without a stiff inheritance tree. Are templates the right way?
  - How can we write "algorithm" code that does not need to know about the ADS, properties, type conversions?

- Would it make sense to a have separate ADS section for each workspace type, such that casting can be avoided?
- The guiding design principle should always be ease of use (intuitive, etc.).
- Workspaces should support chaining multiple algorithms calls, to improve cache use.
- Workspace types are mirrored in property types? If we add new workspaces we also need to add new properties, unless all workspaces inherit from the same class?

# Draft 1

If we do not have an inheritance tree for workspaces, how can we write generic algorithms?
`exploration1.cpp` shows a way, client algorithm code is very simple.

- Will this get cumbersome with many properties?
- Do we need a property system at the algorithm implementation level?
- Do we actually need the ADS in C++?

# Draft 2

No ADS in C++. pybind11 and Python do everything for us?

# Draft 3

- Using `boost::any` is probably just a simpler variant of Draft-1, but runs into the same problem.
- How can we make any overloaded function into an algorithm? Do we need to?
- Should we write algorithms always in two parts, a shell that deals with properties, and normal C++ code doing the actual work?
  - Is making this simple part of the workspace scope? My current feeling is that getting the design wrong could make this difficult.
