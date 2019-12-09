# Tests in this file work only with a working Mantid installation available in
# PYTHONPATH.
import unittest
import pytest

import scipp as sc
import scipp.compat.mantid as mantidcompat
import numpy as np

from mantid_data_helper import MantidDataHelper


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

#    def test_Workspace2D(self):
#        import mantid.simpleapi as mantid
#        eventWS = mantid.CloneWorkspace(self.base_event_ws)
#        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
#        d = mantidcompat.convert_Workspace2D_to_dataarray(ws)
#        self.assertEqual(
#            d.attrs["run"].value.getProperty("run_start").value,
#            "2012-05-21T15:14:56.279289666",
#        )
#
#    def test_EventWorkspace(self):
#        import mantid.simpleapi as mantid
#        eventWS = mantid.CloneWorkspace(self.base_event_ws)
#        ws = mantid.Rebin(eventWS, 10000)
#
#        binned_mantid = mantidcompat.convert_Workspace2D_to_dataarray(ws)
#
#        target_tof = binned_mantid.coords[sc.Dim.Tof]
#        d = mantidcompat.convertEventWorkspace_to_dataarray(
#            eventWS, False)
#        binned = sc.histogram(d, target_tof)
#
#        delta = sc.sum(binned_mantid - binned, sc.Dim.Spectrum)
#        delta = sc.sum(delta, sc.Dim.Tof)
#        self.assertLess(np.abs(delta.value), 1e-5)
#
#    def test_unit_conversion(self):
#        import mantid.simpleapi as mantid
#        eventWS = mantid.CloneWorkspace(self.base_event_ws)
#        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
#        tmp = mantidcompat.convert_Workspace2D_to_dataarray(ws)
#        target_tof = tmp.coords[sc.Dim.Tof]
#        ws = mantid.ConvertUnits(InputWorkspace=ws,
#                                 Target="Wavelength",
#                                 EMode="Elastic")
#        converted_mantid = mantidcompat.convert_Workspace2D_to_dataarray(ws)
#
#        da = mantidcompat.convertEventWorkspace_to_dataarray(
#            eventWS, False)
#        da = sc.histogram(da, target_tof)
#        d = sc.Dataset(da)
#        converted = sc.neutron.convert(d, sc.Dim.Tof, sc.Dim.Wavelength)
#
#        self.assertTrue(
#            np.all(np.isclose(converted_mantid.values, converted[""].values)))
#        self.assertTrue(
#            np.all(
#                np.isclose(
#                    converted_mantid.coords[sc.Dim.Wavelength].values,
#                    converted.coords[sc.Dim.Wavelength].values,
#                )))
#        # delta = sc.sum(converted_mantid - converted, sc.Dim.Spectrum)
#
#    @staticmethod
#    def _mask_bins_and_spectra(ws, xmin, xmax, num_spectra):
#        import mantid.simpleapi as mantid
#        masked_ws = mantid.MaskBins(ws, XMin=xmin, XMax=xmax)
#
#        # mask the first 3 spectra
#        for i in range(num_spectra):
#            masked_ws.spectrumInfo().setMasked(i, True)
#
#        return masked_ws
#
#    def test_Workspace2D_common_bins_masks(self):
#        import mantid.simpleapi as mantid
#        eventWS = mantid.CloneWorkspace(self.base_event_ws)
#        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
#        ws_x = ws.readX(0)
#
#        # mask the first 3 bins, range is taken as [XMin, XMax)
#        masked_ws = self._mask_bins_and_spectra(ws,
#                                                xmin=ws_x[0],
#                                                xmax=ws_x[3],
#                                                num_spectra=3)
#
#        self.assertTrue(masked_ws.isCommonBins())
#
#        ds = mantidcompat.convert_Workspace2D_to_dataarray(masked_ws)
#
#        np.testing.assert_array_equal(
#            ds.masks["bin"].values[0:3],
#            [True, True, True])
#
#        np.testing.assert_array_equal(
#            ds.masks["spectrum"].values[0:3],
#            [True, True, True])
#
#    def test_Workspace2D_not_common_bins_masks(self):
#        import mantid.simpleapi as mantid
#        eventWS = mantid.CloneWorkspace(self.base_event_ws)
#        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
#        ws = mantid.ConvertUnits(ws, "Wavelength",
#                                 EMode="Direct",
#                                 EFixed=0.1231)
#
#        # these X values will mask different number of bins
#        masked_ws = self._mask_bins_and_spectra(ws, -214, -192, num_spectra=3)
#
#        self.assertFalse(masked_ws.isCommonBins())
#
#        ds = mantidcompat.convert_Workspace2D_to_dataarray(masked_ws)
#
#        # bin with 3 masks
#        np.testing.assert_array_equal(
#            ds.masks["bin"].values[0],
#            [True, True, False, False, False])
#
#        # bin with only 2
#        np.testing.assert_array_equal(
#            ds.masks["bin"].values[31],
#            [True, True, True, False, False])
#
#        np.testing.assert_array_equal(
#            ds.masks["spectrum"].values[0:3],
#            [True, True, True])
#
#    def test_Workspace2D_with_separate_monitors(self):
#        filename = MantidDataHelper.find_file("WISH00016748.raw")
#        ds = mantidcompat.load(filename,
#                               mantid_args={"LoadMonitors": "Separate"})
#        monitors = ds.attrs['monitors'].values
#        assert isinstance(monitors, sc.DataArray)
#        assert monitors.shape == [5, 4471]
#
#    def test_Workspace2D_with_include_monitors(self):
#        filename = MantidDataHelper.find_file("WISH00016748.raw")
#        ds = mantidcompat.load(filename,
#                               mantid_args={"LoadMonitors": "Include"})
#        monitors = ds.attrs['monitors'].values
#        assert isinstance(monitors, sc.DataArray)
#        assert monitors.shape == [5, 4471]
#
#    def test_EventWorkspace_with_monitors(self):
#        filename = MantidDataHelper.find_file("CNCS_51936_event.nxs")
#        ds = mantidcompat.load(filename, mantid_args={"LoadMonitors": True})
#        monitors = ds.attrs['monitors'].values
#        assert isinstance(monitors, sc.DataArray)
#        assert monitors.shape == [2, 200001]

    def test_mdhisto_workspace(self):
        from mantid.simpleapi import (CreateMDWorkspace, FakeMDEventData,
                                      BinMD, SaveMD)

        md_event = CreateMDWorkspace(Dimensions=3,
                                     Extents=[-10, 10, -10, 10, -10, 10],
                                     Names='A,B,C',
                                     Units='U,U,U',
                                     StoreInADS=False)
        FakeMDEventData(InputWorkspace=md_event,
                        PeakParams=[100000, 0, 0, 0, 1],
                        StoreInADS=False)  # Add Peak
        md_histo = BinMD(InputWorkspace=md_event,
                         AlignedDim0='A,-10,10,3',
                         AlignedDim1='B,-10,10,4',
                         AlignedDim2='C,-10,10,5',
                         StoreInADS=False)
        file_name = 'md_histo.nxs'
        SaveMD(InputWorkspace=md_histo,
               Filename='md_histo.nxs',
               StoreInADS=False)

        histo_dataset = mantidcompat.load(file_name)

        self.assertEqual(histo_dataset.coords[sc.Dim.X].values.shape, (3, ))
        self.assertEqual(histo_dataset.coords[sc.Dim.Y].values.shape, (4, ))
        self.assertEqual(histo_dataset.coords[sc.Dim.Z].values.shape, (5, ))
        # Sum over 2 dimensions to simplify finding max.
        max_1d = sc.sum(sc.sum(histo_dataset[''], dim=sc.Dim.Y),
                        dim=sc.Dim.X).values
        max_index = np.argmax(max_1d)
        # Check position of max 'peak'
        self.assertEqual(np.floor(len(max_1d) / 2), max_index)
        # All events in central 'peak'
        self.assertEqual(100000, max_1d[max_index])

        self.assertTrue('nevents' in histo_dataset.attrs)

    def test_mdhisto_workspace_q(self):
        from mantid.simpleapi import (CreateMDWorkspace, FakeMDEventData,
                                      BinMD, SaveMD)

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
                         AlignedDim0='Q_x,-10,10,3',
                         AlignedDim1='Q_y,-10,10,4',
                         AlignedDim2='Q_z,-10,10,5',
                         StoreInADS=False)
        file_name = 'md_histo.nxs'
        SaveMD(InputWorkspace=md_histo,
               Filename='md_histo.nxs',
               StoreInADS=False)

        histo_dataset = mantidcompat.load(file_name)

        print(histo_dataset)
        self.assertEqual(histo_dataset.coords[sc.Dim.Qx].values.shape, (3, ))
        self.assertEqual(histo_dataset.coords[sc.Dim.Qy].values.shape, (4, ))
        self.assertEqual(histo_dataset.coords[sc.Dim.Qz].values.shape, (5, ))
        # Sum over 2 dimensions to simplify finding max.
        max_1d = sc.sum(sc.sum(histo_dataset[''], dim=sc.Dim.Qy),
                        dim=sc.Dim.Qx).values
        max_index = np.argmax(max_1d)
        # Check position of max 'peak'
        self.assertEqual(np.floor(len(max_1d) / 2), max_index)
        # All events in central 'peak'
        self.assertEqual(100000, max_1d[max_index])

        self.assertTrue('nevents' in histo_dataset.attrs)
    
#    def test_Workspace2D(self):
#        import mantid.simpleapi as mantid
#        eventWS = mantid.CloneWorkspace(self.base_event_ws)
#        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
#        d = mantidcompat.convert_Workspace2D_to_dataarray(ws)
#        self.assertEqual(
#            d.attrs["run"].value.getProperty("run_start").value,
#            "2012-05-21T15:14:56.279289666",
#        )
#
#    def test_EventWorkspace(self):
#        import mantid.simpleapi as mantid
#        eventWS = mantid.CloneWorkspace(self.base_event_ws)
#        ws = mantid.Rebin(eventWS, 10000)
#
#        binned_mantid = mantidcompat.convert_Workspace2D_to_dataarray(ws)
#
#        target_tof = binned_mantid.coords[sc.Dim.Tof]
#        d = mantidcompat.convertEventWorkspace_to_dataarray(
#            eventWS, False)
#        binned = sc.histogram(d, target_tof)
#
#        delta = sc.sum(binned_mantid - binned, sc.Dim.Spectrum)
#        delta = sc.sum(delta, sc.Dim.Tof)
#        self.assertLess(np.abs(delta.value), 1e-5)
#
#    def test_unit_conversion(self):
#        import mantid.simpleapi as mantid
#        eventWS = mantid.CloneWorkspace(self.base_event_ws)
#        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
#        tmp = mantidcompat.convert_Workspace2D_to_dataarray(ws)
#        target_tof = tmp.coords[sc.Dim.Tof]
#        ws = mantid.ConvertUnits(InputWorkspace=ws,
#                                 Target="Wavelength",
#                                 EMode="Elastic")
#        converted_mantid = mantidcompat.convert_Workspace2D_to_dataarray(ws)
#
#        da = mantidcompat.convertEventWorkspace_to_dataarray(
#            eventWS, False)
#        da = sc.histogram(da, target_tof)
#        d = sc.Dataset(da)
#        converted = sc.neutron.convert(d, sc.Dim.Tof, sc.Dim.Wavelength)
#
#        self.assertTrue(
#            np.all(np.isclose(converted_mantid.values, converted[""].values)))
#        self.assertTrue(
#            np.all(
#                np.isclose(
#                    converted_mantid.coords[sc.Dim.Wavelength].values,
#                    converted.coords[sc.Dim.Wavelength].values,
#                )))
#        # delta = sc.sum(converted_mantid - converted, sc.Dim.Spectrum)
#
#    @staticmethod
#    def _mask_bins_and_spectra(ws, xmin, xmax, num_spectra):
#        import mantid.simpleapi as mantid
#        masked_ws = mantid.MaskBins(ws, XMin=xmin, XMax=xmax)
#
#        # mask the first 3 spectra
#        for i in range(num_spectra):
#            masked_ws.spectrumInfo().setMasked(i, True)
#
#        return masked_ws
#
#    def test_Workspace2D_common_bins_masks(self):
#        import mantid.simpleapi as mantid
#        eventWS = mantid.CloneWorkspace(self.base_event_ws)
#        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
#        ws_x = ws.readX(0)
#
#        # mask the first 3 bins, range is taken as [XMin, XMax)
#        masked_ws = self._mask_bins_and_spectra(ws,
#                                                xmin=ws_x[0],
#                                                xmax=ws_x[3],
#                                                num_spectra=3)
#
#        self.assertTrue(masked_ws.isCommonBins())
#
#        ds = mantidcompat.convert_Workspace2D_to_dataarray(masked_ws)
#
#        np.testing.assert_array_equal(
#            ds.masks["bin"].values[0:3],
#            [True, True, True])
#
#        np.testing.assert_array_equal(
#            ds.masks["spectrum"].values[0:3],
#            [True, True, True])
#
#    def test_Workspace2D_not_common_bins_masks(self):
#        import mantid.simpleapi as mantid
#        eventWS = mantid.CloneWorkspace(self.base_event_ws)
#        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
#        ws = mantid.ConvertUnits(ws, "Wavelength",
#                                 EMode="Direct",
#                                 EFixed=0.1231)
#
#        # these X values will mask different number of bins
#        masked_ws = self._mask_bins_and_spectra(ws, -214, -192, num_spectra=3)
#
#        self.assertFalse(masked_ws.isCommonBins())
#
#        ds = mantidcompat.convert_Workspace2D_to_dataarray(masked_ws)
#
#        # bin with 3 masks
#        np.testing.assert_array_equal(
#            ds.masks["bin"].values[0],
#            [True, True, False, False, False])
#
#        # bin with only 2
#        np.testing.assert_array_equal(
#            ds.masks["bin"].values[31],
#            [True, True, True, False, False])
#
#        np.testing.assert_array_equal(
#            ds.masks["spectrum"].values[0:3],
#            [True, True, True])
#
#    def test_Workspace2D_with_separate_monitors(self):
#        filename = MantidDataHelper.find_file("WISH00016748.raw")
#        ds = mantidcompat.load(filename,
#                               mantid_args={"LoadMonitors": "Separate"})
#        monitors = ds.attrs['monitors'].values
#        assert isinstance(monitors, sc.DataArray)
#        assert monitors.shape == [5, 4471]
#
#    def test_Workspace2D_with_include_monitors(self):
#        filename = MantidDataHelper.find_file("WISH00016748.raw")
#        ds = mantidcompat.load(filename,
#                               mantid_args={"LoadMonitors": "Include"})
#        monitors = ds.attrs['monitors'].values
#        assert isinstance(monitors, sc.DataArray)
#        assert monitors.shape == [5, 4471]
#
#    def test_EventWorkspace_with_monitors(self):
#        filename = MantidDataHelper.find_file("CNCS_51936_event.nxs")
#        ds = mantidcompat.load(filename, mantid_args={"LoadMonitors": True})
#        monitors = ds.attrs['monitors'].values
#        assert isinstance(monitors, sc.DataArray)
#        assert monitors.shape == [2, 200001]
#
#    def test_Workspace2D_with_specific_axis_units(self):
#        filename = MantidDataHelper.find_file("iris26176_graphite002_sqw.nxs")
#        ds = mantidcompat.load(filename)
#        assert ds.shape == [23, 200]
#        assert ds.dims == [sc.Dim.Q, sc.Dim.EnergyTransfer]

if __name__ == "__main__":
    unittest.main()
