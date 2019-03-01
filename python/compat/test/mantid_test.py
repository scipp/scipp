# Tests in this file work only with a working Mantid installation available in PYTHONPATH.
import unittest

from dataset import *
import mantid.simpleapi as mantid
import dataset.compat.mantid as mantidcompat
import numpy as np
#import matplotlib.pyplot as plt

class TestMantidConversion(unittest.TestCase):
    def test_Workspace2D(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        ws = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(ws, -0.001, PreserveEvents=False)
        d = mantidcompat.to_dataset(ws)
        #dataset = as_xarray(d[Dim.Position, 1000:2000])
        #dataset['Value'].plot()
        #plt.show()
        #plt.savefig('test.png')

    def test_EventWorkspace(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        ews = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(ews, -0.001, PreserveEvents=False)

        binned_mantid = mantidcompat.to_dataset(ws)

        tof = Variable(binned_mantid[Coord.Tof])
        d = mantidcompat.to_dataset(ews)
        binned = histogram(d, tof)

        delta = sum(binned_mantid - binned, Dim.Position)

        #ds = as_xarray(delta)
        #ds['Value'].plot()
        #plt.savefig('delta.png')

        #ds = as_xarray(sum(binned, Dim.Position))
        #ds['Value'].plot()
        #plt.savefig('binned.png')

        #ds = as_xarray(sum(binned_mantid, Dim.Position))
        #ds['Value'].plot()
        #plt.savefig('binned_mantid.png')
