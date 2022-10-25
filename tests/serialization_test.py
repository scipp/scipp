# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import scipp as sc
from scipp.serialization import serialize, deserialize
import pytest

_ = pytest.importorskip('h5py')


def test_serialize_roundtrip():
    # No detailed testing since this is currently just using HDF5IO internally
    da = sc.data.binned_x(nevent=10, nbin=2)
    ds = sc.Dataset(data={'a': da})
    assert sc.identical(deserialize(*serialize(da.data)), da.data)
    assert sc.identical(deserialize(*serialize(da)), da)
    assert sc.identical(deserialize(*serialize(ds)), ds)
