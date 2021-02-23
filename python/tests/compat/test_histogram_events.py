from mantid_scipp_comparison import MantidScippComparison
from mantid_data_helper import mantid_is_available
import pytest
import scipp.compat.mantid as mantid
import scipp as sc
import numpy as np


class HistogramEventsTest(MantidScippComparison):
    def __init__(self):
        super(HistogramEventsTest, self).__init__('histogram_events_test')

    @property
    def _filenames(self):
        return {
            "CNCS_51936_event.nxs": ("5ba401e489260a44374b5be12b780911", "MD5")
        }

    def _run_mantid(self, input):
        import mantid.simpleapi as sapi
        # Note Mantid rebin inclusive of last bin boundary
        out = sapi.Rebin(InputWorkspace=input,
                         Params=[0, 10, 1000],
                         PreserveEvents=False,
                         StoreInADS=False)
        return mantid.from_mantid(out)

    def _run_scipp(self, input):
        return sc.histogram(x=input,
                            bins=sc.Variable(dims=['tof'],
                                             values=np.arange(0, 1010, 10),
                                             dtype=sc.dtype.float64,
                                             unit=sc.units.us))


@pytest.mark.skipif(not mantid_is_available(),
                    reason='Mantid framework is unavailable')
def test_histogram_events():
    rebin = HistogramEventsTest()
    print(rebin.run(allow_failure=True))
