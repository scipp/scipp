# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
import numpy as np
from .common import assert_export


class TestDatasetSlice:
    def setup_method(self):
        var = sc.Variable(['x'], values=np.arange(5, dtype=np.int64))
        self._d = sc.Dataset(data={'a': var, 'b': var}, coords={'x': var})

    def test_slice_with_range_datasetview_then_dataarrayview(self):
        sl = self._d['x', 1:-1]['a'].data
        ref = sc.Variable(['x'], values=np.array([1, 2, 3], dtype=np.int64))
        assert ref == sl
        # omitting range end
        sl = self._d['x', 1:]['a'].data
        ref = sc.Variable(['x'], values=np.array([1, 2, 3, 4], dtype=np.int64))
        assert ref == sl
        # omitting range begin
        sl = self._d['x', :-1]['a'].data
        ref = sc.Variable(['x'], values=np.array([0, 1, 2, 3], dtype=np.int64))
        assert ref == sl
        # omitting range both begin and end
        sl = self._d['x', :]['b'].data
        ref = sc.Variable(['x'],
                          values=np.array([0, 1, 2, 3, 4], dtype=np.int64))
        assert ref == sl

    def test_slice_with_range_dataarrayview_then_dataarrayview(self):
        sl = self._d['a']['x', 1:-1].data
        ref = sc.Variable(['x'], values=np.array([1, 2, 3], dtype=np.int64))
        assert ref == sl
        # omitting range end
        sl = self._d['a']['x', 1:].data
        ref = sc.Variable(['x'], values=np.array([1, 2, 3, 4], dtype=np.int64))
        assert ref == sl
        # omitting range begin
        sl = self._d['a']['x', :-1].data
        ref = sc.Variable(['x'], values=np.array([0, 1, 2, 3], dtype=np.int64))
        assert ref == sl
        # omitting range both begin and end
        sl = self._d['b']['x', :].data
        ref = sc.Variable(['x'],
                          values=np.array([0, 1, 2, 3, 4], dtype=np.int64))
        assert ref == sl

    def test_slice_single_index(self):
        assert self._d['x', -2]['a'] == self._d['x', 3]['a']
        assert self._d['a']['x', -2] == self._d['a']['x', 3]

    def _test_copy_exports_on(self, x):
        assert_export(x.copy)
        assert_export(x.__copy__)
        assert_export(x.__deepcopy__, {})

    def test_copy_datasetview_exports(self):
        d = sc.Dataset()
        view = sc.DatasetView(d)
        self._test_copy_exports_on(view)

    def test_copy_dataarrayview_exports(self):
        view = self._d['a']
        self._test_copy_exports_on(view)

    def test_set_item_via_temporary_slice(self):
        N = 6
        M = 4
        d1 = sc.Dataset()
        d1['x'] = sc.Variable(['x'],
                              values=np.arange(N + 1).astype(np.float64))
        d1['y'] = sc.Variable(['y'],
                              values=np.arange(M + 1).astype(np.float64))
        arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
        d1['a'] = sc.Variable(['x', 'y'], values=arr1)
        d1 = d1['x', 1:2]
        d1['a'].data.values.tolist() == [[5.0, 6.0, 7.0, 8.0]]

    def test_set_dataarrayview_slice_items(self):
        d = self._d.copy()
        d['a']['x', 0:2] += d['b']['x', 0:2]
        assert d['a'].data.values.tolist() == [0, 2, 2, 3, 4]
        d['a']['x', 4] += \
            d['b']['x', 1]
        assert d['a'].data.values.tolist() == [0, 2, 2, 3, 5]

    def test_slice_and_dimensions_items_dataarray(self):
        var = sc.Variable(['x', 'y'], values=np.arange(50).reshape(5, 10))
        da = sc.DataArray(var)
        assert np.allclose(da['x', 0].values, da['x', 0:1].values)
        assert np.allclose(da['x', 4].values, da['x', -1].values)
        assert np.allclose(da['y', 1].values, da['y', -9].values)
        assert ('y' in da['x', 0].dims)
        assert ('x' not in da['x', 0].dims)
        assert ('y' in da['x', 0:1].dims)
        assert ('x' in da['x', 0:1].dims)

    def test_slice_and_dimensions_items_dataset(self):
        da = sc.DataArray(
            sc.Variable(['x', 'y'], values=np.arange(50).reshape(5, 10)))
        ds = sc.Dataset({'a': da})
        assert (np.allclose(ds['x', 0]['a'].values,
                            ds['x', 0:1]['a'].values[0],
                            atol=1e-9))
        assert (np.allclose(ds['x', 4]['a'].values, ds['x', -1]['a'].values))
        assert (np.allclose(ds['y', 1]['a'].values, ds['y', -9]['a'].values))
        assert ('y' in da['x', 0].dims)
        assert ('x' not in da['x', 0].dims)
        assert ('y' in da['x', 0:1].dims)
        assert ('x' in da['x', 0:1].dims)

    def test_slice_dataset_with_data_only(self):
        d = sc.Dataset()
        d['data'] = sc.Variable(['y'], values=np.arange(10))
        sliced = d['y', :]
        assert d == sliced
        sliced = d['y', 2:6]
        assert sc.Variable(['y'], values=np.arange(2,
                                                   6)) == sliced['data'].data

    def test_slice_dataset_with_coords_only(self):
        d = sc.Dataset()
        d.coords['y-coord'] = sc.Variable(['y'], values=np.arange(10))
        sliced = d['y', :]
        assert d == sliced
        sliced = d['y', 2:6]
        assert sc.Variable(['y'],
                           values=np.arange(2, 6)) == sliced.coords['y-coord']

    def test_slice_dataset_with_attrs_only(self):
        d = sc.Dataset()
        d.attrs['y-attr'] = sc.Variable(['y'], values=np.arange(10))
        sliced = d['y', :]
        assert d == sliced
        sliced = d['y', 2:6]
        assert sc.Variable(['y'],
                           values=np.arange(2, 6)) == sliced.attrs['y-attr']

    def test_slice_dataset_with_masks_only(self):
        d = sc.Dataset()
        d.masks['y-mask'] = sc.Variable(['y'], values=np.arange(10))
        sliced = d['y', :]
        assert d == sliced
        sliced = d['y', 2:6]
        assert sc.Variable(['y'],
                           values=np.arange(2, 6)) == sliced.masks['y-mask']
