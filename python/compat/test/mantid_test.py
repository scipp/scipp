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
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, -0.001, PreserveEvents=False)
        d = mantidcompat.to_dataset(ws)
        #dataset = as_xarray(d[Dim.Position, 1000:2000])
        #dataset['Value'].plot()
        #plt.show()
        #plt.savefig('test.png')

    def test_EventWorkspace(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, -0.001, PreserveEvents=False)

        binned_mantid = mantidcompat.to_dataset(ws)

        tof = Variable(binned_mantid[Coord.Tof])
        d = mantidcompat.to_dataset(eventWS)
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

    def test_unit_conversion(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, -0.001, PreserveEvents=False)
        tmp = mantidcompat.to_dataset(ws)
        tof = Variable(tmp[Coord.Tof])
        ws = mantid.ConvertUnits(InputWorkspace=ws, Target='DeltaE', EMode='Direct', EFixed=3.3056)

        converted_mantid = mantidcompat.to_dataset(ws)
        converted_mantid[Coord.Ei] = ([], 3.3059)

        d = mantidcompat.to_dataset(eventWS, drop_pulse_times=True)
        d[Coord.Ei] = ([], 3.3059)
        d.merge(histogram(d, tof))
        del(d[Data.Events])
        converted = convert(d, Dim.Tof, Dim.DeltaE)

        delta = sum(converted_mantid - converted, Dim.Position)

        # ds = as_xarray(delta)
        # ds['Value'].plot()

        # ds = as_xarray(sum(converted, Dim.Position))
        # ds['Value'].plot()

        # ds = as_xarray(sum(converted_mantid, Dim.Position))
        # ds['Value'].plot()
        # plt.savefig('converted.png')

if __name__ == '__main__':
    unittest.main()
