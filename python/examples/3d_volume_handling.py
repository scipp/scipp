# To convert to jupyter notebook type: tpy2nb path/filename.py 
# result file path is: path/filename.ipynb 
# py2nb link: https://github.com/williamjameshandley/py2nb
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
# Rebin the X axis
d = rebin(d, Variable(Coord.X, [Dim.X], np.arange(0, L+1, 2).astype(np.float64)))
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
