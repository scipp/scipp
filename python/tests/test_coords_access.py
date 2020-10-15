# Note this is a temporary test file

import scipp as sc
import numpy as np

def test_coords_access():
    v = sc.Variable(['x'], values=np.arange(2))
    da = sc.DataArray(v, coords={'x':v})
    assert da.coords.unaligned_coords == da.unaligned_coords
    assert da.coords.aligned_coords == da.aligned_coords
    assert da.coords() == da.coords_old

