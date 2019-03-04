# To convert to jupyter notebook type: tpy2nb path/filename.py 
# result file path is: path/filename.ipynb 
# py2nb link: https://github.com/williamjameshandley/py2nb
#|
# Imports
from dataset import *
import numpy as np
#----------------------

#|

d = Dataset()
d[Coord.SpectrumNumber] = ([Dim.Spectrum], np.arange(1, 101))
#-------------------------------------------------------------

#|
# Add a (common) time-of-flight axis
d[Coord.Tof] = ([Dim.Tof], np.arange(1000))
#--------------------------------------------

#|
# Add data with uncertainties
d[Data.Value, "sample1"] = ([Dim.Spectrum, Dim.Tof], np.random.exponential(size=100*1000).reshape([100, 1000]))
d[Data.Variance, "sample1"] = d[Data.Value, "sample1"]
#---------------------------------------------------------

#|
# Create a mask and use it to extract some of the spectra
select = Variable(Coord.Mask, [Dim.Spectrum], np.isin(d[Coord.SpectrumNumber], np.arange(10, 20)))
spectra = filter(d, select)
#--------------------------------------------------------------------------------------

#|
# Direct representation of a simple instrument (more standard Mantid instrument
# representation is of course supported, this is just to demonstrate the flexibility)
steps = np.arange(-0.45, 0.46, 0.1)
x = np.tile(steps,(10,))
y = x.reshape([10,10]).transpose().flatten()
d[Coord.X] = ([Dim.Spectrum], x)
d[Coord.Y] = ([Dim.Spectrum], y)
d[Coord.Z] = ([], 10.0)
#----------------------------------------------------

#|
# Mask some spectra based on distance from beam center
r = np.sqrt(np.square(d[Coord.X]) + np.square(d[Coord.Y]))
d[Coord.Mask] = ([Dim.Spectrum], np.less(r, 0.2))
#--------------------------------------------------------

#|
# Do something for each spectrum (here: apply mask)
d[Coord.Mask].data
for i, masked in enumerate(d[Coord.Mask].numpy):
    spec = d[Dim.Spectrum, i]
    if masked:
        spec[Data.Value, "sample1"] = np.zeros(1000)
        spec[Data.Variance, "sample1"] = np.zeros(1000)
#-------------------------------------------------------        
