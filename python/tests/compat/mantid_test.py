# Tests in this file work only with a working Mantid installation available in
# PYTHONPATH.
import unittest

import scipp as sc
import mantid.simpleapi as mantid
from mantid.api import EventType
import scipp.compat.mantid as mantidcompat
import numpy as np


class TestMantidConversion(unittest.TestCase):
    def test_Workspace2D(self):
        # This is from the Mantid system-test data
        filename = "CNCS_51936_event.nxs"
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        d = mantidcompat.convert_Workspace2D_to_dataset(ws)
        self.assertEqual(
            d.attrs["run"].value.getProperty("run_start").value,
            "2012-05-21T15:14:56.279289666",
        )

    def test_EventWorkspace(self):
        # This is from the Mantid system-test data
        filename = "CNCS_51936_event.nxs"
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, 10000)

        binned_mantid = mantidcompat.convert_Workspace2D_to_dataset(ws)

        target_tof = binned_mantid.coords[sc.Dim.Tof]
        d = mantidcompat.convert_EventWorkspace_to_dataset(
            eventWS, False, EventType.TOF)
        binned = sc.histogram(d, target_tof)

        delta = sc.sum(binned_mantid - binned, sc.Dim.Spectrum)
        delta = sc.sum(delta, sc.Dim.Tof)
        self.assertLess(np.abs(delta.value), 1e-5)

    def test_unit_conversion(self):
        # This is from the Mantid system-test data
        filename = "CNCS_51936_event.nxs"
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        tmp = mantidcompat.convert_Workspace2D_to_dataset(ws)
        target_tof = tmp.coords[sc.Dim.Tof]
        ws = mantid.ConvertUnits(InputWorkspace=ws,
                                 Target="Wavelength",
                                 EMode="Elastic")
        converted_mantid = mantidcompat.convert_Workspace2D_to_dataset(ws)

        da = mantidcompat.convert_EventWorkspace_to_dataset(
            eventWS, False, EventType.TOF)
        da = sc.histogram(da, target_tof)
        d = sc.Dataset(da)
        converted = sc.neutron.convert(d, sc.Dim.Tof, sc.Dim.Wavelength)

        self.assertTrue(
            np.all(np.isclose(converted_mantid.values, converted[""].values)))
        self.assertTrue(
            np.all(
                np.isclose(
                    converted_mantid.coords[sc.Dim.Wavelength].values,
                    converted.coords[sc.Dim.Wavelength].values,
                )))
        # delta = sc.sum(converted_mantid - converted, sc.Dim.Spectrum)


if __name__ == "__main__":
    unittest.main()
