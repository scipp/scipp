# Tests in this file work only with a working Mantid installation available in PYTHONPATH.
import unittest

from dataset import *
from mantid.simpleapi import *
import dataset.compat.mantid as mantidcompat
import numpy as np
#import matplotlib.pyplot as plt

class TestMantidConversion(unittest.TestCase):
    def test_Workspace2D(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        ws = LoadEventNexus(filename)
        ws = Rebin(ws, -0.001, PreserveEvents=False)
        d = mantidcompat.to_dataset(ws)
        #dataset = as_xarray(d[Dim.Position, 1000:2000])
        #dataset['Value'].plot()
        #plt.show()
        #plt.savefig('test.png')
