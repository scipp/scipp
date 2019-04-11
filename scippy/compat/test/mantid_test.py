# Tests in this file work only with a working Mantid installation available in
# PYTHONPATH.
import unittest

import scippy as sp
import mantid.simpleapi as mantid
import scippy.compat.mantid as mantidcompat
# import matplotlib.pyplot as plt


class TestMantidConversion(unittest.TestCase):

    def test_Workspace2D(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, -0.001, PreserveEvents=False)
        d = mantidcompat.to_dataset(ws)
        print(d)
        # self.assertEqual(d.dimensions.shape[])
        # dataset = as_xarray(d[sp.Dim.Position, 1000:2000])
        # dataset['Value'].plot()
        # plt.show()
        # plt.savefig('test.png')

    def test_EventWorkspace(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, -0.001, PreserveEvents=False)

        binned_mantid = mantidcompat.to_dataset(ws)

        tof = sp.Variable(binned_mantid[sp.Coord.Tof])
        d = mantidcompat.to_dataset(eventWS)
        binned = sp.histogram(d, tof)

        delta = sp.sum(binned_mantid - binned, sp.Dim.Position)
        print(delta)

        # ds = as_xarray(delta)
        # ds['Value'].plot()
        # plt.savefig('delta.png')

        # ds = as_xarray(sum(binned, sp.Dim.Position))
        # ds['Value'].plot()
        # plt.savefig('binned.png')

        # ds = as_xarray(sum(binned_mantid, sp.Dim.Position))
        # ds['Value'].plot()
        # plt.savefig('binned_mantid.png')

    def test_unit_conversion(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, -0.001, PreserveEvents=False)
        tmp = mantidcompat.to_dataset(ws)
        tof = sp.Variable(tmp[sp.Coord.Tof])
        ws = mantid.ConvertUnits(InputWorkspace=ws, Target='DeltaE',
                                 EMode='Direct', EFixed=3.3056)

        converted_mantid = mantidcompat.to_dataset(ws)
        converted_mantid[sp.Coord.Ei] = ([], 3.3059)

        d = mantidcompat.to_dataset(eventWS, drop_pulse_times=True)
        d[sp.Coord.Ei] = ([], 3.3059)
        d.merge(sp.histogram(d, tof))
        del(d[sp.Data.Events])
        converted = sp.convert(d, sp.Dim.Tof, sp.Dim.DeltaE)

        delta = sp.sum(converted_mantid - converted, sp.Dim.Position)
        print(delta)

        # ds = as_xarray(delta)
        # ds['Value'].plot()

        # ds = as_xarray(sum(converted, sp.Dim.Position))
        # ds['Value'].plot()

        # ds = as_xarray(sum(converted_mantid, sp.Dim.Position))
        # ds['Value'].plot()
        # plt.savefig('converted.png')


if __name__ == '__main__':
    unittest.main()
