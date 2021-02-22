from mantid_data_helper import MantidDataHelper
import mantid.simpleapi as sapi
import scipp.compat.mantid as mantid
import scipp as sc
import time


class Comparison:
    def _execute_with_timing(self, op, **kwargs):
        start = time.time()
        result = op(**kwargs)
        stop = time.time()
        return result, stop - start

    def run(self):
        results = []
        for hash, algorithm in self._filenames.items():
            file = MantidDataHelper.find_file(hash, algorithm)
            in_ws = sapi.Load(Filename=file, StoreInADS=False)
            out_mantid_da, time_mantid = self._execute_with_timing(
                self._run_mantid, workspace=in_ws)
            in_da = mantid.from_mantid(in_ws).astype(
                sc.dtype.float64)  # Converters set weights float32
            out_scipp_da, time_scipp = self._execute_with_timing(
                self._run_scipp, data_array=in_da)
            assert sc.is_equal(out_mantid_da, out_scipp_da)
            results.append((out_scipp_da, out_mantid_da))
        return results

    @property
    def _filenames(self):
        pass

    def _run_mantid(self, **kwargs):
        raise RuntimeError("_run_mantid not implemented in base")

    def _run_scipp(self, **kwargs):
        raise RuntimeError("_run_scipp not implemented in base")
