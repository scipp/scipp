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
`draft1.cpp` shows a way, client algorithm code is very simple.

- Will this get cumbersome with many properties?
- Do we need a property system at the algorithm implementation level?
- Do we actually need the ADS in C++?

# Draft 2

No ADS in C++. pybind11 and Python do everything for us?

# Draft 3

- Using `boost::any` is probably just a simpler variant of Draft-1, but runs into the same problem.
- How can we make any overloaded function into an algorithm? Do we need to?
- Should we write algorithms always in two parts, a shell that deals with properties, and normal C++ code doing the actual work?
  - Is making this simple part of the workspace scope? My current feeling is that getting the workspace design wrong could make this difficult on the algorithm side, so I believe we must at the very least come up with a working proof of concept for this.

# Draft 4

- What do algorithms actually need? Most do not need, e.g., instrument, or `SpectrumInfo`. Table of L2, etc. might be enough.
  - See for example discussion at developer meeting on `ConvertUnits` not working for constant-wavelength instruments -- only wavelength is needed.
  - Can we select the right overload depending on what information is available?
- We have to avoid duplicating concepts.
  - Example1: Masking. Currently there is `MaskWorkspace` as well as masking on the instrument/workspace.
  - Can we eliminate one of these? Either masking should always be stored in workspace, or always we a separate workspace (provided to algorithms by hand, or used automatically when linked).
  - Same for `GroupingWorkspace`: We have grouping information in the workspace, and we have workspaces defining grouping patterns. Eliminate one.
  - If we eliminate the workspaces, provide a different way for users to view that masking/grouping information.
- Should the instrument be handled in the same way as discussed for masking and grouping above?
- How do we modify such linked workspaces, such as `MaskWorkspace`?
  - If directly, should it always takes a copy (break all links)? This would add overhead of copying, and later setting a new link from data workspace to the modified mask workspace.
  - If directly but not copy, how can we possibly avoid breaking data workspaces that link?
  - If via link, take a copy (if necessary, there might just be a single link), link stays intact, but the flow of logic is not nice (modify one object via another)
  - Forbid changing grouping in existing workspace? This does usually not make sense.
- Are the `Axis` objects duplicating information that is contained in `Histogram` and the spectrum numbers (if we generalize to more generic spectrum labels)? What happens on transpose? X might need generic labels (actually `Transpose` does not support that!)?

# Iterator (`iterator.cpp`)

- Iterators that provide a unified view of information linked to by workspace, including data, spectrum labels, spectrum definitions, masking.
  - Low-level algorithms to work with iterators?
- If we link to workspaces from iterators, access should most likely be only `const` (never modify linked workspaces via these iterators, there are *data* iterators).
- Workspaces are "the same" for some algorithms but different types for others.
  - For example, `Rebin` can work on anything that contains histograms, but histograms could be linked to detector pixels (positions), a two-theta value, Q, or nothing.
  - An algorithm like `ConvertUnits` does need to handle these differently.
  - Can we and should we express this via the type system?
  - One way is at the iterator level. Open questions: How to create output workspace. Whichever piece of code gets the iterators needs to know the actual type, unless we use inheritance to handle this?
  - Is a better way via a view ("overlay")?
  - Inheritance?
  - Algorithms should just run on the relevant sub-object!

# Composition (`composition.cpp`)

- Shows how to write algorithms for sub-components.
