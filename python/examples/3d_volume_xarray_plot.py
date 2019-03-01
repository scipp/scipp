#| 
# Imports
from dataset import *
import numpy as np
# Uncomment fo jupyter
#%matplotlib inline
#-------------

#| 
# Create empty dataset
d = Dataset()
#-----------------------

#| 
# Put randon temperature in 3d volume
L = 30
d[Coord.X] = ([Dim.X], np.arange(L).astype(np.float64))
d[Coord.Y] = ([Dim.Y], np.arange(L).astype(np.float64))
d[Coord.Z] = ([Dim.Z], np.arange(L).astype(np.float64))
d[Data.Value, "temperature"] = ([Dim.X, Dim.Y, Dim.Z], np.random.normal(size=L*L*L).reshape([L,L,L]))

#| 
# Convert to xarray and plot
xr_ds = as_xarray(d)
xr_ds['Value:temperature'][10, ...].plot()
#----------------------------------------
