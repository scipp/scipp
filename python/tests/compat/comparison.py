from mantid_data_helper import MantidDataHelper
import mantid.simpleapi as sapi
import scipp.compat.mantid as mantid
import scipp as sc
import time


class Comparison:
    def __init__(self, test_description=None):
        self._test_description = test_description

    def _execute_with_timing(self, op, **kwargs):
        start = time.time()
        result = op(**kwargs)
        stop = time.time()
        return result, (stop - start) * sc.Unit('s')

    def run(self):
        results = sc.Dataset()
        for i, (name, (hash, algorithm)) in enumerate(self._filenames.items()):
            file = MantidDataHelper.find_file(hash, algorithm)
            print('Loading', name)
            in_ws = sapi.Load(Filename=file, StoreInADS=False)
            out_mantid_da, time_mantid = self._execute_with_timing(
                self._run_mantid, workspace=in_ws)
            in_da = mantid.from_mantid(in_ws).astype(
                sc.dtype.float64)  # Converters set weights float32
            out_scipp_da, time_scipp = self._execute_with_timing(
                self._run_scipp, data_array=in_da)
            assert sc.is_equal(out_mantid_da, out_scipp_da)

            result = sc.DataArray(sc.equal(out_mantid_da, out_scipp_da).data,
                                  coords={
                                      'diff':
                                      out_mantid_da.data - out_scipp_da.data,
                                      'is_approx':
                                      sc.is_approx(out_mantid_da.data,
                                                   out_scipp_da.data,
                                                   1e-9 * sc.Unit('counts')),
                                      'duration_scipp':
                                      time_scipp,
                                      'duration_mantid':
                                      time_mantid
                                  })

            results[f'{i}' if self._test_description is None else
                    f'{self._test_description}_{i}'] = result
        return results

    @property
    def _filenames(self):
        pass

    def _run_mantid(self, **kwargs):
        raise RuntimeError("_run_mantid not implemented in base")

    def _run_scipp(self, **kwargs):
        raise RuntimeError("_run_scipp not implemented in base")
