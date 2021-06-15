import scipp as sc
import numpy as np
import types


v = sc.array(dims=['x'], values=[-1, 2, -4, 3])
da = sc.DataArray(v, coords={'x': sc.array(dims='x', values=[1,-2,3, 4])})

# print(da.max())

print(v.all.__doc__)
print("=================================================")
print(sc.all.__doc__)
print("=================================================")
print(v.__doc__)
print("|||||||||||||||||||||||||||||||||||||||||||||||||")
print("|||||||||||||||||||||||||||||||||||||||||||||||||")
help(v.all)
print("=================================================")
help(sc.all)
print("=================================================")
help(v)
# help(types.FunctionType)