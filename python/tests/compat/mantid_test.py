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
        filename = 'CNCS_51936_event.nxs'
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        d = mantidcompat.convert_Workspace2D_to_dataset(ws)
        self.assertEqual(d.labels['run'].value.getProperty('run_start').value, '2012-05-21T15:14:56.279289666')

    def test_EventWorkspace(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, 10000)

        binned_mantid = mantidcompat.convert_Workspace2D_to_dataset(ws)
	
        target_tof = binned_mantid.coords[sc.Dim.Tof]
        d = mantidcompat.convert_EventWorkspace_to_dataset(eventWS, False, EventType.TOF)
        binned = sc.histogram(d, target_tof)

        # Following label removal should not be necessary
        del binned_mantid.labels['run']
        del binned_mantid.labels['sample']
        del binned.labels['run']
        del binned.labels['sample']
        delta = sc.sum(binned_mantid - binned, sc.Dim.Spectrum)

    def test_unit_conversion(self):
        # This is from the Mantid system-test data
        filename = 'CNCS_51936_event.nxs'
        eventWS = mantid.LoadEventNexus(filename)
        ws = mantid.Rebin(eventWS, 10000, PreserveEvents=False)
        tmp = mantidcompat.convert_Workspace2D_to_dataset(ws)
        target_tof = tmp.coords[sc.Dim.Tof] 
        ws = mantid.ConvertUnits(InputWorkspace=ws, Target='Wavelength',
                                 EMode='Elastic')
        converted_mantid = mantidcompat.convert_Workspace2D_to_dataset(ws)

        da = mantidcompat.convert_EventWorkspace_to_dataset(eventWS, False, EventType.TOF)
        da = sc.histogram(da, target_tof)
        d = sc.Dataset(da)
        converted = sc.neutron.convert(d, sc.Dim.Tof, sc.Dim.Wavelength)

        # Following label removal should not be necessary
        del converted_mantid.labels['run']
        del converted_mantid.labels['sample']
        del converted.labels['run']
        del converted.labels['sample']

        print(converted_mantid)
        self.assertTrue(np.all(np.isclose(converted_mantid.values, converted[''].values)))
        self.assertTrue(np.all(np.isclose(converted_mantid.coords[sc.Dim.Wavelength].values, converted.coords[sc.Dim.Wavelength].values)))
        #delta = sc.sum(converted_mantid - converted, sc.Dim.Spectrum)


if __name__ == '__main__':
    unittest.main()
