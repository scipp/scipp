Key problems to solve:
- Link various information together, including instrument, logs, related workspaces, masking.
- How can we write generic algorithms without a stiff inheritance tree. Are templates the right way?

- Would it make sense to a have separate ADS section for each workspace type, such that casting can be avoided?
- The guiding design principle should always be ease of use (intuitive, etc.).

# Draft 1

If we do not have an inheritance tree for workspaces, how can we write generic algorithms?
`exploration1.cpp` shows a way, client algorithm code is very simple.

- Will this get cumbersome with many properties?
- Do we need a property system at the algorithm implementation level?
- Do we actually need the ADS in C++?
