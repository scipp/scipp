from comparison import Comparison
from mantid_data_helper import mantid_is_available
import pytest
import mantid.simpleapi as sapi
import scipp.compat.mantid as mantid
import scipp as sc
import numpy as np


class RebinTest(Comparison):
    @property
    def _filenames(self):
        return {"5ba401e489260a44374b5be12b780911": "MD5"}

    def _run_mantid(self, **kwargs):
        out = sapi.Rebin(kwargs['workspace'],
                         Params=[0, 10, 1000],
                         PreserveEvents=False,
                         StoreInADS=False)
        out_da = mantid.from_mantid(out)
        return out_da

    def _run_scipp(self, **kwargs):
        return sc.bin(kwargs['data_array'],
                      edges=[
                          sc.Variable(dims=['tof'],
                                      values=np.arange(0, 1010, 10),
                                      dtype=sc.dtype.float64,
                                      unit=sc.units.us)
                      ]).bins.sum()


@pytest.mark.skipif(not mantid_is_available(),
                    reason='Mantid framework is unavailable')
def test_rebin():
    rebin = RebinTest()
    rebin.run()
