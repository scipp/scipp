# Tests in this file work only with a working Mantid installation available in
# PYTHONPATH.
import unittest

import scipp as sc
import mantid.simpleapi as mantid
import scipp.compat.mantid as mantidcompat
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
        # dataset = as_xarray(d[sc.Dim.Position, 1000:2000])
        # dataset['Value'].plot()
        # plt.show()
        # plt.savefig('test.png')

    def test_EventWorkspace(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, -0.001, PreserveEvents=False)

        binned_mantid = mantidcompat.to_dataset(ws)

        tof = sc.Variable(binned_mantid[sc.Coord.Tof])
        d = mantidcompat.to_dataset(eventWS)
        binned = sc.histogram(d, tof)

        delta = sc.sum(binned_mantid - binned, sc.Dim.Position)
        print(delta)

        # ds = as_xarray(delta)
        # ds['Value'].plot()
        # plt.savefig('delta.png')

        # ds = as_xarray(sum(binned, sc.Dim.Position))
        # ds['Value'].plot()
        # plt.savefig('binned.png')

        # ds = as_xarray(sum(binned_mantid, sc.Dim.Position))
        # ds['Value'].plot()
        # plt.savefig('binned_mantid.png')

    def test_unit_conversion(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, -0.001, PreserveEvents=False)
        tmp = mantidcompat.to_dataset(ws)
        tof = sc.Variable(tmp[sc.Coord.Tof])
        ws = mantid.ConvertUnits(InputWorkspace=ws, Target='DeltaE',
                                 EMode='Direct', EFixed=3.3056)

        converted_mantid = mantidcompat.to_dataset(ws)
        converted_mantid[sc.Coord.Ei] = ([], 3.3059)

        d = mantidcompat.to_dataset(eventWS, drop_pulse_times=True)
        d[sc.Coord.Ei] = ([], 3.3059)
        d.merge(sc.histogram(d, tof))
        del(d[sc.Data.Events])
        converted = sc.convert(d, sc.Dim.Tof, sc.Dim.DeltaE)

        delta = sc.sum(converted_mantid - converted, sc.Dim.Position)
        print(delta)

        # ds = as_xarray(delta)
        # ds['Value'].plot()

        # ds = as_xarray(sum(converted, sc.Dim.Position))
        # ds['Value'].plot()

        # ds = as_xarray(sum(converted_mantid, sc.Dim.Position))
        # ds['Value'].plot()
        # plt.savefig('converted.png')


if __name__ == '__main__':
    unittest.main()
