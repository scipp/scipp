# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest
import scipp as sc


def make_var() -> sc.Variable:
    return sc.arange('dummy', 12, dtype='int64').fold(dim='dummy',
                                                      sizes={
                                                          'xx': 4,
                                                          'yy': 3
                                                      })


def make_array() -> sc.DataArray:
    da = sc.DataArray(make_var())
    da.coords['xx'] = sc.arange('xx', 4, dtype='int64')
    da.coords['yy'] = sc.arange('yy', 3, dtype='int64')
    return da


def make_dataset() -> sc.Dataset:
    ds = sc.Dataset()
    ds['xy'] = make_array()
    ds['x'] = ds.coords['xx']
    return ds


@pytest.mark.parametrize("obj", [make_var(), make_array(), make_dataset()])
@pytest.mark.parametrize("pos", [0, 1, 2, 3, -2, -3, -4])
def test_length_1_list_gives_corresponding_length_1_slice(obj, pos):
    assert sc.identical(obj['xx', [pos]], obj['xx', pos:pos + 1])


@pytest.mark.parametrize("obj", [make_var(), make_array(), make_dataset()])
def test_every_other_index_gives_stride_2_slice(obj):
    assert sc.identical(obj['xx', [0, 2]], obj['xx', 0::2])
    assert sc.identical(obj['xx', [1, 3]], obj['xx', 1::2])


@pytest.mark.parametrize("obj", [make_var(), make_array(), make_dataset()])
def test_unordered_indices_yields_result_with_reordered_slices(obj):
    assert sc.identical(obj['xx', [2, 3, 0]],
                        sc.concat([obj['xx', 2:4], obj['xx', 0:1]], 'xx'))


@pytest.mark.parametrize("obj", [make_var(), make_array(), make_dataset()])
def test_duplicate_indices_duplicate_slices_in_output(obj):
    s1 = obj['xx', 1:2]
    s2 = obj['xx', 2:3]
    assert sc.identical(obj['xx', [2, 1, 1, 2]], sc.concat([s2, s1, s1, s2], 'xx'))


@pytest.mark.parametrize("obj", [make_var(), make_array(), make_dataset()])
def test_reversing_twice_gives_original(obj):
    assert sc.identical(obj['xx', [3, 2, 1, 0]]['xx', [3, 2, 1, 0]], obj)


@pytest.mark.skip("Inconsistent behavior with slice by mask but consistent with concat")
def test_bin_edges_are_dropped():
    da = make_array()
    base = da.copy()
    da.coords['edges'] = sc.concat([da.coords['xx'], da.coords['xx'][-1] + 1], 'xx')
    assert sc.identical(da['xx', [0, 2, 3]],
                        sc.concat([base['xx', 0], base['xx', 2:]], 'xx'))


def test_dataset_item_independent_of_slice_dim_preserved_unchanged():
    ds = make_dataset()
    assert sc.identical(ds['yy', [0, 2]]['x'], ds['x'])


def test_2d_list_raises_TypeError():
    var = make_var()
    with pytest.raises(TypeError):
        var['xx', [[0], [2]]]


@pytest.mark.parametrize("pos", [-6, -5, 4, 5])
def test_out_of_range_index_raises_IndexError(pos):
    var = sc.arange('xx', 4)
    with pytest.raises(IndexError):
        var['xx', pos]
