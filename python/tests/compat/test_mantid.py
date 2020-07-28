# Tests in this file work only with a working Mantid installation available in
# PYTHONPATH.
import unittest

import numpy as np
import pytest

import scipp as sc
from mantid_data_helper import MantidDataHelper
from scipp.compat import mantid as mantidcompat


def mantid_is_available():
    try:
        import mantid  # noqa: F401
        return True
    except ImportError:
        return False


def memory_is_at_least_gb(required):
    import psutil
    total = psutil.virtual_memory().total / 1e9
    return total >= required


@pytest.mark.skipif(not memory_is_at_least_gb(16),
                    reason='Insufficient virtual memory')
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
            OutputWorkspace="test_ws{}".format(__file__),
            StoreInADS=False)

    def test_Workspace2D(self):
        import mantid.simpleapi as mantid
        eventWS = mantid.CloneWorkspace(self.base_event_ws)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        d = mantidcompat.convert_Workspace2D_to_data_array(ws)
        self.assertEqual(
            d.attrs["run"].value.getProperty("run_start").value,
            "2012-05-21T15:14:56.279289666",
        )
        self.assertEqual(d.data.unit, sc.units.counts)
        for i in range(ws.getNumberHistograms()):
            assert np.all(np.equal(d.values[i], ws.readY(i)))
            assert np.all(np.equal(d.variances[i], ws.readE(i) * ws.readE(i)))
        self.assertEqual(d.coords['spectrum'].dtype, sc.dtype.int32)
        self.assertEqual(d.coords['tof'].dtype, sc.dtype.float64)

    def test_EventWorkspace(self):
        import mantid.simpleapi as mantid
        eventWS = mantid.CloneWorkspace(self.base_event_ws)
        ws = mantid.Rebin(eventWS, 10000)

        binned_mantid = mantidcompat.convert_Workspace2D_to_data_array(ws)

        target_tof = binned_mantid.coords['tof']
        d = mantidcompat.convert_EventWorkspace_to_data_array(
            eventWS, load_pulse_times=False)
        d.realign({'tof': target_tof})
        binned = sc.histogram(d)

        delta = sc.sum(binned_mantid - binned, 'spectrum')
        delta = sc.sum(delta, 'tof')
        self.assertLess(np.abs(delta.value), 1e-5)

    def test_EventWorkspace_realign_events(self):
        import mantid.simpleapi as mantid
        eventWS = mantid.CloneWorkspace(self.base_event_ws)

        realigned = mantidcompat.convert_EventWorkspace_to_data_array(
            eventWS, realign_events=True, load_pulse_times=False)

        d = mantidcompat.convert_EventWorkspace_to_data_array(
            eventWS, realign_events=False, load_pulse_times=False)
        d.realign({'tof': realigned.coords['tof']})

        # Removing run and sample due to missing comparison operators
        del d.attrs['run']
        del d.attrs['sample']
        del realigned.attrs['run']
        del realigned.attrs['sample']
        assert realigned == d

    def test_comparison(self):
        a = mantidcompat.convert_EventWorkspace_to_data_array(
            self.base_event_ws, load_pulse_times=False)
        b = a.copy()
        assert a == b

    def test_EventWorkspace_no_y_unit(self):
        import mantid.simpleapi as mantid
        tiny_event_ws = mantid.CreateSampleWorkspace(WorkspaceType='Event',
                                                     NumBanks=1,
                                                     NumEvents=1)
        d = mantidcompat.convert_EventWorkspace_to_data_array(
            tiny_event_ws, load_pulse_times=False)
        self.assertEqual(d.data.unit, sc.units.counts)
        tiny_event_ws.setYUnit('')
        d = mantidcompat.convert_EventWorkspace_to_data_array(
            tiny_event_ws, load_pulse_times=False)
        self.assertEqual(d.data.unit, sc.units.dimensionless)

    def test_from_mantid_LoadEmptyInstrument(self):
        import mantid.simpleapi as mantid
        ws = mantid.LoadEmptyInstrument(InstrumentName='PG3')
        mantidcompat.from_mantid(ws)

    def test_from_mantid_CreateWorkspace(self):
        import mantid.simpleapi as mantid
        dataX = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]
        dataY = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
        ws = mantid.CreateWorkspace(DataX=dataX,
                                    DataY=dataY,
                                    NSpec=4,
                                    UnitX="Wavelength")
        d = mantidcompat.from_mantid(ws)
        self.assertEqual(d.data.unit, sc.units.dimensionless)

    def test_unit_conversion(self):
        import mantid.simpleapi as mantid
        eventWS = mantid.CloneWorkspace(self.base_event_ws)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        tmp = mantidcompat.convert_Workspace2D_to_data_array(ws)
        target_tof = tmp.coords['tof']
        ws = mantid.ConvertUnits(InputWorkspace=ws,
                                 Target="Wavelength",
                                 EMode="Elastic")
        converted_mantid = mantidcompat.convert_Workspace2D_to_data_array(ws)

        da = mantidcompat.convert_EventWorkspace_to_data_array(
            eventWS, load_pulse_times=False)
        da.realign({'tof': target_tof})
        da = sc.histogram(da)
        d = sc.Dataset(da)
        converted = sc.neutron.convert(d, 'tof', 'wavelength')

        self.assertTrue(
            np.all(np.isclose(converted_mantid.values, converted[""].values)))
        self.assertTrue(
            np.all(
                np.isclose(
                    converted_mantid.coords['wavelength'].values,
                    converted.coords['wavelength'].values,
                )))
        # delta = sc.sum(converted_mantid - converted, 'spectrum')

    @staticmethod
    def _mask_bins_and_spectra(ws, xmin, xmax, num_spectra, indices=None):
        import mantid.simpleapi as mantid
        masked_ws = mantid.MaskBins(ws,
                                    XMin=xmin,
                                    XMax=xmax,
                                    InputWorkspaceIndexSet=indices)

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

    def test_Workspace2D_common_bins_not_common_masks(self):
        import mantid.simpleapi as mantid
        eventWS = mantid.CloneWorkspace(self.base_event_ws)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        ws_x = ws.readX(0)

        # mask first 3 bins in first 3 spectra, range is taken as [XMin, XMax)
        masked_ws = self._mask_bins_and_spectra(ws,
                                                xmin=ws_x[0],
                                                xmax=ws_x[3],
                                                num_spectra=3,
                                                indices='0-2')

        self.assertTrue(masked_ws.isCommonBins())

        ds = mantidcompat.convert_Workspace2D_to_data_array(masked_ws)

        mask = sc.Variable(dims=ds.dims, shape=ds.shape, dtype=sc.dtype.bool)
        mask['spectrum', 0:3]['tof', 0:3] |= sc.Variable(value=True)
        assert ds.masks['bin'] == mask

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
        masked_ws = self._mask_bins_and_spectra(ws,
                                                -214,
                                                -192,
                                                num_spectra=3,
                                                indices='0-40')

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
        from mantid.simpleapi import mtd
        mtd.clear()
        filename = MantidDataHelper.find_file("WISH00016748.raw")
        ds = mantidcompat.load(filename,
                               mantid_args={"LoadMonitors": "Separate"})
        self.assertEqual(len(mtd), 0, mtd.getObjectNames())
        attrs = ds.attrs.keys()
        expected_monitor_attrs = set(
            ["monitor1", "monitor2", "monitor3", "monitor4", "monitor5"])
        assert expected_monitor_attrs.issubset(attrs)

        for monitor_name in expected_monitor_attrs:
            monitors = ds.attrs[monitor_name].values
            assert isinstance(monitors, sc.DataArray)
            assert monitors.shape == [4471]

    def test_Workspace2D_with_include_monitors(self):
        from mantid.simpleapi import mtd
        mtd.clear()
        filename = MantidDataHelper.find_file("WISH00016748.raw")
        ds = mantidcompat.load(filename,
                               mantid_args={"LoadMonitors": "Include"})
        self.assertEqual(len(mtd), 0, mtd.getObjectNames())
        attrs = ds.attrs.keys()
        expected_monitor_attrs = set(
            ["monitor1", "monitor2", "monitor3", "monitor4", "monitor5"])
        assert expected_monitor_attrs.issubset(attrs)
        for monitor_name in expected_monitor_attrs:
            monitors = ds.attrs[monitor_name].values
            assert isinstance(monitors, sc.DataArray)
            assert monitors.shape == [4471]

    def test_EventWorkspace_with_monitors(self):
        from mantid.simpleapi import mtd
        mtd.clear()
        filename = MantidDataHelper.find_file("CNCS_51936_event.nxs")
        ds = mantidcompat.load(filename, mantid_args={"LoadMonitors": True})
        self.assertEqual(len(mtd), 0, mtd.getObjectNames())
        attrs = ds.attrs.keys()
        expected_monitor_attrs = set(["monitor2", "monitor3"])
        assert expected_monitor_attrs.issubset(attrs)
        for monitor_name in expected_monitor_attrs:
            monitor = ds.attrs[monitor_name].value
            assert isinstance(monitor, sc.DataArray)
            assert monitor.shape == [200001]
            assert 'position' in monitor.coords
            assert 'source-position' in monitor.coords
            # This is essential, otherwise unit conversion assumes scattering
            # from sample:
            assert 'sample-position' not in monitor.coords
            # Absence of the following is not crucial, but currently there is
            # no need for these, and it avoid duplication:
            assert 'detector-info' not in monitor.coords
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

        self.assertEqual(histo_data_array.coords['Q_x'].values.shape, (4, ))
        self.assertEqual(histo_data_array.coords['Q_y'].values.shape, (3, ))
        self.assertEqual(histo_data_array.coords['Q_z'].values.shape, (5, ))
        self.assertEqual(histo_data_array.coords['Q_x'].unit,
                         sc.units.dimensionless / sc.units.angstrom)
        self.assertEqual(histo_data_array.coords['Q_y'].unit,
                         sc.units.dimensionless / sc.units.angstrom)
        self.assertEqual(histo_data_array.coords['Q_z'].unit,
                         sc.units.dimensionless / sc.units.angstrom)

        self.assertEquals(histo_data_array.values.shape, (3, 4, 5))

        # Sum over 2 dimensions to simplify finding max.
        max_1d = sc.sum(sc.sum(histo_data_array, dim='Q_y'), dim='Q_x').values
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
        from mantid.simpleapi import mtd
        mtd.clear()

        ds = sc.Dataset()

        sc.compat.mantid.load_component_info(
            ds, MantidDataHelper.find_file("iris26176_graphite002_sqw.nxs"))

        # check that no workspaces have been leaked in the ADS
        assert len(mtd) == 0, f"Workspaces present: {mtd.getObjectNames()}"

        self.assertTrue("source-position" in ds.coords)
        self.assertTrue("sample-position" in ds.coords)
        self.assertTrue("position" in ds.coords)

    def test_to_workspace_2d_no_error(self):
        from mantid.simpleapi import mtd
        mtd.clear()

        # All Dims for which support is expected are
        # tested in the parametrized test.
        # Just set this one to a working one to avoid
        # generating many repetitive tests.
        param_dim = 'tof'
        data_len = 2
        expected_bins = data_len + 1
        expected_number_spectra = 10

        y = sc.Variable(['spectrum', param_dim],
                        values=np.random.rand(expected_number_spectra,
                                              data_len))

        x = sc.Variable(['spectrum', param_dim],
                        values=np.arange(
                            expected_number_spectra * expected_bins,
                            dtype=np.float64).reshape(
                                (expected_number_spectra, expected_bins)))
        data = sc.DataArray(data=y, coords={param_dim: x})

        ws = sc.compat.mantid.to_mantid(data, param_dim)

        assert len(ws.readX(0)) == expected_bins
        assert ws.getNumberHistograms() == expected_number_spectra
        # check that no workspaces have been leaked in the ADS
        assert len(mtd) == 0, f"Workspaces present: {mtd.getObjectNames()}"

        for i in range(expected_number_spectra):
            np.testing.assert_array_equal(ws.readX(i), x['spectrum', i])
            np.testing.assert_array_equal(ws.readY(i), y['spectrum', i])
            np.testing.assert_array_equal(ws.readE(i),
                                          np.sqrt(y['spectrum', i].values))

    def test_fit(self):
        """
        Tests that the fit executes, and the outputs
        are moved into the dataset. Does not check the fit values.
        """
        from mantid.simpleapi import mtd
        mtd.clear()

        data = sc.neutron.load(filename=MantidDataHelper.find_file(
            "iris26176_graphite002_sqw.nxs"))

        params, diff = sc.compat.mantid.fit(
            data['Q', 0],
            mantid_args={
                'Function': 'name=LinearBackground,A0=0,A1=1',
                'StartX': 0,
                'EndX': 3
            })

        # check that no workspaces have been leaked in the ADS
        assert len(mtd) == 0
        assert 'data' in diff
        assert 'calculated' in diff
        assert 'diff' in diff
        assert 'status' in params.attrs
        assert 'function' in params.attrs
        assert 'cost-function' in params.attrs
        assert 'chi^2/d.o.f.' in params.attrs

    def test_set_run(self):
        import mantid.simpleapi as mantid
        target = mantid.CloneWorkspace(self.base_event_ws)
        d = mantidcompat.convert_EventWorkspace_to_data_array(target, False)
        d.attrs["run"].value.addProperty("test_property", 1, True)
        # before
        self.assertFalse(target.run().hasProperty("test_property"))
        target.setRun(d.attrs["run"].value)
        # after
        self.assertTrue(target.run().hasProperty("test_property"))

    def test_set_sample(self):
        import mantid.simpleapi as mantid
        target = mantid.CloneWorkspace(self.base_event_ws)
        d = mantidcompat.convert_EventWorkspace_to_data_array(target, False)
        d.attrs["sample"].value.setThickness(3)
        # before
        self.assertNotEqual(3, target.sample().getThickness())
        target.setSample(d.attrs["sample"].value)
        # after
        self.assertEqual(3, target.sample().getThickness())

    def _do_test_point(self, point):
        x, y, z = point
        r, theta, phi = mantidcompat._to_spherical(x, y, z)
        x_b, y_b, z_b = mantidcompat._to_cartesian(r, theta, phi)
        self.assertAlmostEqual(x, x_b)
        self.assertAlmostEqual(y, y_b)
        self.assertAlmostEqual(z, z_b)

    def test_detector_positions(self):
        import mantid.simpleapi as mantid
        from mantid.kernel import V3D
        eventWS = mantid.CloneWorkspace(self.base_event_ws)
        comp_info = eventWS.componentInfo()
        small_offset = V3D(0.01, 0.01, 0.01)
        comp_info.setPosition(comp_info.source(),
                              comp_info.samplePosition() + small_offset)
        moved = mantidcompat.convert_Workspace2D_to_data_array(eventWS)
        moved_det_position = moved.coords["position"]
        unmoved = mantidcompat.convert_Workspace2D_to_data_array(eventWS)
        unmoved_det_positions = unmoved.coords["position"]
        # Moving the sample accounted for in position calculations
        # but should not yield change to final detector positions
        self.assertTrue(
            np.all(
                np.isclose(moved_det_position.values,
                           unmoved_det_positions.values)))

    def test_validate_units(self):
        acceptable = ["wavelength", sc.Dim.Wavelength]
        for i in acceptable:
            ret = mantidcompat.validate_dim_and_get_mantid_string(i)
            self.assertEqual(ret, "Wavelength")

    def test_validate_units_throws(self):
        not_acceptable = [None, "None", "wavlength", 1, 1.0, ["wavelength"]]
        for i in not_acceptable:
            with self.assertRaises(RuntimeError):
                mantidcompat.validate_dim_and_get_mantid_string(i)


def test_to_rot_from_vectors():
    a = sc.Variable(value=[1, 0, 0], dtype=sc.dtype.vector_3_float64)
    b = sc.Variable(value=[0, 1, 0], dtype=sc.dtype.vector_3_float64)
    rot = mantidcompat._rot_from_vectors(a.value, b.value)
    assert np.allclose((rot * a).value, b.value)
    rot = mantidcompat._rot_from_vectors(b.value, a.value)
    assert np.allclose((rot * b).value, a.value)


@pytest.mark.skipif(not memory_is_at_least_gb(16),
                    reason='Insufficient virtual memory')
@pytest.mark.skipif(not mantid_is_available(),
                    reason='Mantid framework is unavailable')
@pytest.mark.parametrize(
    "param_dim",
    ('tof', 'wavelength', 'E', 'd-spacing', 'Q', 'Q^2', 'Delta-E'))
def test_to_workspace_2d(param_dim):
    from mantid.simpleapi import mtd
    mtd.clear()

    data_len = 2
    expected_bins = data_len + 1
    expected_number_spectra = 10

    y = sc.Variable(['spectrum', param_dim],
                    values=np.random.rand(expected_number_spectra, data_len),
                    variances=np.random.rand(expected_number_spectra,
                                             data_len))

    x = sc.Variable(['spectrum', param_dim],
                    values=np.arange(expected_number_spectra * expected_bins,
                                     dtype=np.float64).reshape(
                                         (expected_number_spectra,
                                          expected_bins)))
    data = sc.DataArray(data=y, coords={param_dim: x})

    ws = sc.compat.mantid.to_mantid(data, param_dim)

    assert len(ws.readX(0)) == expected_bins
    assert ws.getNumberHistograms() == expected_number_spectra
    # check that no workspaces have been leaked in the ADS
    assert len(mtd) == 0, f"Workspaces present: {mtd.getObjectNames()}"

    for i in range(expected_number_spectra):
        np.testing.assert_array_equal(ws.readX(i), x['spectrum', i])
        np.testing.assert_array_equal(ws.readY(i), y['spectrum', i])
        np.testing.assert_array_equal(ws.readE(i),
                                      np.sqrt(y['spectrum', i].variances))


@pytest.mark.skipif(not mantid_is_available(),
                    reason='Mantid framework is unavailable')
def test_to_workspace_2d_handles_single_spectra():
    from mantid.simpleapi import mtd
    mtd.clear()

    expected_x = [0., 1., 2.]
    expected_y = [10., 20., 30.]
    expected_e = [4., 4., 4.]

    x = sc.Variable(['tof'], values=expected_x)
    y = sc.Variable(['tof'], values=expected_y, variances=expected_e)
    data = sc.DataArray(data=y, coords={'tof': x})

    ws = sc.compat.mantid.to_mantid(data, "tof")

    assert ws.getNumberHistograms() == 1

    assert np.equal(ws.readX(0), expected_x).all()
    assert np.equal(ws.readY(0), expected_y).all()
    assert np.equal(ws.readE(0), np.sqrt(expected_e)).all()


@pytest.mark.skipif(not mantid_is_available(),
                    reason='Mantid framework is unavailable')
def test_to_workspace_2d_handles_single_x_array():
    from mantid.simpleapi import mtd
    mtd.clear()

    expected_x = [0., 1., 2.]
    expected_y = [[10., 20., 30.], [40., 50., 60.]]
    expected_e = [[4., 4., 4.], [4., 4., 4.]]

    x = sc.Variable(['tof'], values=expected_x)
    y = sc.Variable(['spectrum', 'tof'],
                    values=np.array(expected_y),
                    variances=np.array(expected_e))
    data = sc.DataArray(data=y, coords={'tof': x})

    ws = sc.compat.mantid.to_mantid(data, "tof")

    assert ws.getNumberHistograms() == 2
    assert np.equal(ws.readX(0), expected_x).all()
    assert np.equal(ws.readX(1), expected_x).all()

    for i, (y_vals, e_vals) in enumerate(zip(expected_y, expected_e)):
        assert np.equal(ws.readY(i), y_vals).all()
        assert np.equal(ws.readE(i), np.sqrt(e_vals)).all()


if __name__ == "__main__":
    unittest.main()
