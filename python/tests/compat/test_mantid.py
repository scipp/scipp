# Tests in this file work only with a working Mantid installation available in
# PYTHONPATH.
import unittest

import numpy as np
import pytest

import scipp as sc
from mantid_data_helper import MantidDataHelper
from scipp import Dim
from scipp.compat import mantid as mantidcompat


def mantid_is_available():
    try:
        import mantid  # noqa: F401
        return True
    except ImportError:
        return False


@pytest.mark.skipif(not mantid_is_available(),
                    reason='Mantid framework is unavailable')
class TestMantidConversion(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import mantid.simpleapi as mantid
        # This is from the Mantid system-test data
        filename = "CNCS_51936_event.nxs"
        # This needs OutputWorkspace specified, as it doesn't
        # pick up the name from the class variable name
        cls.base_event_ws = mantid.LoadEventNexus(
            MantidDataHelper.find_file(filename),
            OutputWorkspace="test_ws{}".format(__file__))

    def test_Workspace2D(self):
        import mantid.simpleapi as mantid
        eventWS = mantid.CloneWorkspace(self.base_event_ws)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        d = mantidcompat.convert_Workspace2D_to_data_array(ws)
        self.assertEqual(
            d.attrs["run"].value.getProperty("run_start").value,
            "2012-05-21T15:14:56.279289666",
        )

    def test_EventWorkspace(self):
        import mantid.simpleapi as mantid
        eventWS = mantid.CloneWorkspace(self.base_event_ws)
        ws = mantid.Rebin(eventWS, 10000)

        binned_mantid = mantidcompat.convert_Workspace2D_to_data_array(ws)

        target_tof = binned_mantid.coords[sc.Dim.Tof]
        d = mantidcompat.convert_EventWorkspace_to_data_array(eventWS, False)
        binned = sc.histogram(d, target_tof)

        delta = sc.sum(binned_mantid - binned, sc.Dim.Spectrum)
        delta = sc.sum(delta, sc.Dim.Tof)
        self.assertLess(np.abs(delta.value), 1e-5)

    def test_unit_conversion(self):
        import mantid.simpleapi as mantid
        eventWS = mantid.CloneWorkspace(self.base_event_ws)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        tmp = mantidcompat.convert_Workspace2D_to_data_array(ws)
        target_tof = tmp.coords[sc.Dim.Tof]
        ws = mantid.ConvertUnits(InputWorkspace=ws,
                                 Target="Wavelength",
                                 EMode="Elastic")
        converted_mantid = mantidcompat.convert_Workspace2D_to_data_array(ws)

        da = mantidcompat.convert_EventWorkspace_to_data_array(eventWS, False)
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

    @staticmethod
    def _mask_bins_and_spectra(ws, xmin, xmax, num_spectra):
        import mantid.simpleapi as mantid
        masked_ws = mantid.MaskBins(ws, XMin=xmin, XMax=xmax)

        # mask the first 3 spectra
        for i in range(num_spectra):
            masked_ws.spectrumInfo().setMasked(i, True)

        return masked_ws

    def test_Workspace2D_common_bins_masks(self):
        import mantid.simpleapi as mantid
        eventWS = mantid.CloneWorkspace(self.base_event_ws)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        ws_x = ws.readX(0)

        # mask the first 3 bins, range is taken as [XMin, XMax)
        masked_ws = self._mask_bins_and_spectra(ws,
                                                xmin=ws_x[0],
                                                xmax=ws_x[3],
                                                num_spectra=3)

        self.assertTrue(masked_ws.isCommonBins())

        ds = mantidcompat.convert_Workspace2D_to_data_array(masked_ws)

        np.testing.assert_array_equal(ds.masks["bin"].values[0:3],
                                      [True, True, True])

        np.testing.assert_array_equal(ds.masks["spectrum"].values[0:3],
                                      [True, True, True])

    def test_Workspace2D_not_common_bins_masks(self):
        import mantid.simpleapi as mantid
        eventWS = mantid.CloneWorkspace(self.base_event_ws)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        ws = mantid.ConvertUnits(ws,
                                 "Wavelength",
                                 EMode="Direct",
                                 EFixed=0.1231)

        # these X values will mask different number of bins
        masked_ws = self._mask_bins_and_spectra(ws, -214, -192, num_spectra=3)

        self.assertFalse(masked_ws.isCommonBins())

        ds = mantidcompat.convert_Workspace2D_to_data_array(masked_ws)

        # bin with 3 masks
        np.testing.assert_array_equal(ds.masks["bin"].values[0],
                                      [True, True, False, False, False])

        # bin with only 2
        np.testing.assert_array_equal(ds.masks["bin"].values[31],
                                      [True, True, True, False, False])

        np.testing.assert_array_equal(ds.masks["spectrum"].values[0:3],
                                      [True, True, True])

    def test_Workspace2D_with_separate_monitors(self):
        filename = MantidDataHelper.find_file("WISH00016748.raw")
        ds = mantidcompat.load(filename,
                               mantid_args={"LoadMonitors": "Separate"})
        attrs = ds.attrs.keys()
        expected_monitor_attrs = set(
            ["monitor1", "monitor2", "monitor3", "monitor4", "monitor5"])
        assert expected_monitor_attrs.issubset(attrs)

        for monitor_name in expected_monitor_attrs:
            monitors = ds.attrs[monitor_name].values
            assert isinstance(monitors, sc.DataArray)
            assert monitors.shape == [4471]

    def test_Workspace2D_with_include_monitors(self):
        filename = MantidDataHelper.find_file("WISH00016748.raw")
        ds = mantidcompat.load(filename,
                               mantid_args={"LoadMonitors": "Include"})
        attrs = ds.attrs.keys()
        expected_monitor_attrs = set(
            ["monitor1", "monitor2", "monitor3", "monitor4", "monitor5"])
        assert expected_monitor_attrs.issubset(attrs)
        for monitor_name in expected_monitor_attrs:
            monitors = ds.attrs[monitor_name].values
            assert isinstance(monitors, sc.DataArray)
            assert monitors.shape == [4471]

    def test_EventWorkspace_with_monitors(self):
        filename = MantidDataHelper.find_file("CNCS_51936_event.nxs")
        ds = mantidcompat.load(filename, mantid_args={"LoadMonitors": True})
        attrs = ds.attrs.keys()
        expected_monitor_attrs = set(["monitor2", "monitor3"])
        assert expected_monitor_attrs.issubset(attrs)
        for monitor_name in expected_monitor_attrs:
            monitor = ds.attrs[monitor_name].value
            assert isinstance(monitor, sc.DataArray)
            assert monitor.shape == [200001]
            assert 'position' in monitor.labels
            assert 'source_position' in monitor.labels
            # This is essential, otherwise unit conversion assumes scattering
            # from sample:
            assert 'sample_position' not in monitor.labels
            # Absence of the following is not crucial, but currently there is
            # no need for these, and it avoid duplication:
            assert 'detector_info' not in monitor.labels
            assert 'run' not in monitor.attrs
            assert 'sample' not in monitor.attrs

    def test_mdhisto_workspace_q(self):
        from mantid.simpleapi import (CreateMDWorkspace, FakeMDEventData,
                                      BinMD)

        md_event = CreateMDWorkspace(Dimensions=3,
                                     Extents=[-10, 10, -10, 10, -10, 10],
                                     Names='Q_x,Q_y,Q_z',
                                     Units='U,U,U',
                                     Frames='QLab,QLab,QLab',
                                     StoreInADS=False)
        FakeMDEventData(InputWorkspace=md_event,
                        PeakParams=[100000, 0, 0, 0, 1],
                        StoreInADS=False)  # Add Peak
        md_histo = BinMD(InputWorkspace=md_event,
                         AlignedDim0='Q_y,-10,10,3',
                         AlignedDim1='Q_x,-10,10,4',
                         AlignedDim2='Q_z,-10,10,5',
                         StoreInADS=False)

        histo_data_array = mantidcompat.convert_MDHistoWorkspace_to_data_array(
            md_histo)

        self.assertEqual(histo_data_array.coords[sc.Dim.Qx].values.shape,
                         (4, ))
        self.assertEqual(histo_data_array.coords[sc.Dim.Qy].values.shape,
                         (3, ))
        self.assertEqual(histo_data_array.coords[sc.Dim.Qz].values.shape,
                         (5, ))
        self.assertEqual(histo_data_array.coords[sc.Dim.Qx].unit,
                         sc.units.dimensionless / sc.units.angstrom)
        self.assertEqual(histo_data_array.coords[sc.Dim.Qy].unit,
                         sc.units.dimensionless / sc.units.angstrom)
        self.assertEqual(histo_data_array.coords[sc.Dim.Qz].unit,
                         sc.units.dimensionless / sc.units.angstrom)

        self.assertEquals(histo_data_array.values.shape, (3, 4, 5))

        # Sum over 2 dimensions to simplify finding max.
        max_1d = sc.sum(sc.sum(histo_data_array, dim=sc.Dim.Qy),
                        dim=sc.Dim.Qx).values
        max_index = np.argmax(max_1d)
        # Check position of max 'peak'
        self.assertEqual(np.floor(len(max_1d) / 2), max_index)
        # All events in central 'peak'
        self.assertEqual(100000, max_1d[max_index])

        self.assertTrue('nevents' in histo_data_array.attrs)

    def test_mdhisto_workspace_many_dims(self):
        from mantid.simpleapi import (CreateMDWorkspace, FakeMDEventData,
                                      BinMD)

        md_event = CreateMDWorkspace(
            Dimensions=4,
            Extents=[-10, 10, -10, 10, -10, 10, -10, 10],
            Names='deltae,y,z,T',
            Units='U,U,U,U',
            StoreInADS=False)
        FakeMDEventData(InputWorkspace=md_event,
                        PeakParams=[100000, 0, 0, 0, 0, 1],
                        StoreInADS=False)  # Add Peak
        md_histo = BinMD(InputWorkspace=md_event,
                         AlignedDim0='deltae,-10,10,3',
                         AlignedDim1='y,-10,10,4',
                         AlignedDim2='z,-10,10,5',
                         AlignedDim3='T,-10,10,7',
                         StoreInADS=False)

        histo_data_array = mantidcompat.convert_MDHistoWorkspace_to_data_array(
            md_histo)
        self.assertEqual(4, len(histo_data_array.dims))

    def test_load_component_info(self):
        ds = sc.Dataset()

        sc.compat.mantid.load_component_info_from_instrument_file(
            ds, MantidDataHelper.find_file("iris26176_graphite002_sqw.nxs"))

        self.assertTrue("source_position" in ds.labels)
        self.assertTrue("sample_position" in ds.labels)
        self.assertTrue("position" in ds.labels)


@pytest.mark.skipif(not mantid_is_available(),
                    reason='Mantid framework is unavailable')
@pytest.mark.parametrize("param_dim",
                         (
                             Dim.Tof,
                             Dim.Wavelength
                         )
                         )
def test_to_mantid(param_dim):
    from mantid import mtd

    data_len = 2
    expected_bins = data_len + 1
    expected_number_spectra = 10

    y = sc.Variable([param_dim.Spectrum, param_dim],
                    values=np.random.rand(expected_number_spectra, data_len),
                    variances=np.random.rand(
        expected_number_spectra, data_len))

    x = sc.Variable([param_dim], values=np.arange(
        expected_bins, dtype=np.float64))

    expected_ws_name = "scipp_to_ws"

    sc.compat.mantid.to_mantid(
        x.values, y.values, y.variances, x.dims[0], expected_ws_name)

    ws = mtd[expected_ws_name]

    assert len(ws.readX(0)) == expected_bins
    assert ws.getNumberHistograms() == expected_number_spectra

    np.testing.assert_array_equal(ws.readX(0), x.values)
    np.testing.assert_array_equal(ws.readY(0), y[Dim.Spectrum, 0])
    np.testing.assert_array_equal(
        ws.readE(0), y[Dim.Spectrum, 0].variances)


def test_to_mantid_no_error():
    x = sc.Variable([Dim.X], values=np.arange(100))
    x = sc.Variable([Dim.X], values=np.arange(100))
    x = sc.Variable([Dim.X], values=np.arange(100))


if __name__ == "__main__":
    unittest.main()
