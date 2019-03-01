#|
# Imports
from dataset import *
import numpy as np
# Uncomment fo jupyter
#%matplotlib inline
#-------------

#|
L = 30
d = Dataset()
#------------------

#|
# Add bin-edge axis for X
d[Coord.X] = ([Dim.X], np.arange(L+1))
# ... and normal axes for Y and Z
d[Coord.Y] = ([Dim.Y], np.arange(L))
d[Coord.Z] = ([Dim.Z], np.arange(L))
#--------------------------------------

#|
# Add data variables
d[Data.Value, "temperature"] = ([Dim.Z, Dim.Y, Dim.X], np.random.normal(size=L*L*L).reshape([L,L,L]))
d[Data.Value, "pressure"] = ([Dim.Z, Dim.Y, Dim.X], np.random.normal(size=L*L*L).reshape([L,L,L]))
# Add uncertainties, matching name implicitly links it to corresponding data
d[Data.Variance, "temperature"] = d[Data.Value, "temperature"]
#----------------------------------------------------------------------------------------------------

#|
# Uncertainties are propagated using grouping mechanism based on name
square = d * d
#--------------------------------------------------------------------

#|
# Add the counts units
d[Data.Value, "temperature"].unit = units.counts
d[Data.Value, "pressure"].unit = units.counts
d[Data.Variance, "temperature"].unit = units.counts * units.counts
# The square operation is now prevented because the resulting counts
# variance unit (counts^4) is not part of the supported units, i.e. the
# result of that operation makes little physical sense.
with self.assertRaisesRegex(RuntimeError, "Unsupported unit as result of multiplication counts\^2\*counts\^2"):
    square = d * d
#--------------------------------------------------------------------------

#|
# Rebin the X axis
d = rebin(d, Variable(Coord.X, [Dim.X], np.arange(0, L+1, 2).astype(np.float64)))
# Rebin to different axis for every y
rebinned = rebin(d, Variable(Coord.X, [Dim.Y, Dim.X], np.arange(0, 2*L).reshape([L,2]).astype(np.float64)))
#------------------------------------------------------------------------------------------------------------

#|
# Do something with numpy and insert result
d[Data.Value, "dz(p)"] = ([Dim.Z, Dim.Y, Dim.X], np.gradient(d[Data.Value, "pressure"], d[Coord.Z], axis=0))
#------------------------------------------------------------------------------------------------------------

#|
# Truncate Y and Z axes
d = Dataset(d[Dim.Y, 10:20][Dim.Z, 10:20])
#--------------------------------------------

#|
# Mean along Y axis
meanY = mean(d, Dim.Y)
# Subtract from original, making use of automatic broadcasting
d -= meanY
#------------

#|
# Extract a Z slice
sliceZ = Dataset(d[Dim.Z, 7])
#-----------------------------
