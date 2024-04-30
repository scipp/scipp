# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from copy import copy, deepcopy

import pytest

import scipp as sc


def data_array_components():
    v = sc.array(dims=['x'], values=[10, 20], unit='m')
    c = sc.array(dims=['x'], values=[1, 2], unit='s')
    a = sc.array(dims=['x'], values=[100, 200])
    m = sc.array(dims=['x'], values=[True, False])
    return v, c, a, m


@pytest.fixture(
    params=(
        lambda k, v: {k: v},
        lambda k, v: {k: v}.items(),
        lambda k, v: sc.DataArray(v, coords={k: v}).coords,
    ),
    ids=['dict', 'iterator', 'Coords'],
)
def coords_arg_wrapper(request):
    return request.param


@pytest.fixture(
    params=(
        lambda k, v: {k: v},
        lambda k, v: {k: v}.items(),
        lambda k, v: sc.DataArray(v, attrs={k: v}).attrs,
    ),
    ids=['dict', 'iterator', 'Coords'],
)
def attrs_arg_wrapper(request):
    return request.param


@pytest.fixture(
    params=(
        lambda k, v: {k: v},
        lambda k, v: {k: v}.items(),
        lambda k, v: sc.DataArray(v, masks={k: v}).masks,
    ),
    ids=['dict', 'iterator', 'Coords'],
)
def masks_arg_wrapper(request):
    return request.param


@pytest.fixture()
def data_array_and_components(coords_arg_wrapper, attrs_arg_wrapper, masks_arg_wrapper):
    v, c, a, m = data_array_components()
    da = sc.DataArray(
        v,
        coords=coords_arg_wrapper('x', c),
        attrs=attrs_arg_wrapper('a', a),
        masks=masks_arg_wrapper('m', m),
    )
    return da, v, c, a, m


def make_data_array():
    v, c, a, m = data_array_components()
    da = sc.DataArray(v, coords={'x': c}, attrs={'a': a}, masks={'m': m})
    return da, v, c, a, m


def test_own_darr_set(data_array_and_components):
    # Data and metadata are shared
    da, v, c, a, m = data_array_and_components
    da['x', 0] = -10
    da.data['x', 1] = -20
    da.coords['x']['x', 0] = -1
    da.attrs['a']['x', 0] = -100
    da.masks['m']['x', 0] = False
    c['x', 1] = -2
    a['x', 1] = -200
    m['x', 1] = True
    da.unit = 'kg'
    da.coords['x'].unit = 'J'
    assert sc.identical(
        da,
        sc.DataArray(
            sc.array(dims=['x'], values=[-10, -20], unit='kg'),
            coords={'x': sc.array(dims=['x'], values=[-1, -2], unit='J')},
            attrs={'a': sc.array(dims=['x'], values=[-100, -200])},
            masks={'m': sc.array(dims=['x'], values=[False, True])},
        ),
    )
    assert sc.identical(v, sc.array(dims=['x'], values=[-10, -20], unit='kg'))
    assert sc.identical(c, sc.array(dims=['x'], values=[-1, -2], unit='J'))
    assert sc.identical(a, sc.array(dims=['x'], values=[-100, -200]))
    assert sc.identical(m, sc.array(dims=['x'], values=[False, True]))

    # Assignments overwrite data but not metadata.
    da.data = sc.array(dims=['x'], values=[11, 22], unit='m')
    da.coords['x'] = sc.array(dims=['x'], values=[3, 4], unit='s')
    da.attrs['a'] = sc.array(dims=['x'], values=[300, 400])
    da.masks['m'] = sc.array(dims=['x'], values=[True, True])
    assert sc.identical(
        da,
        sc.DataArray(
            sc.array(dims=['x'], values=[11, 22], unit='m'),
            coords={'x': sc.array(dims=['x'], values=[3, 4], unit='s')},
            attrs={'a': sc.array(dims=['x'], values=[300, 400])},
            masks={'m': sc.array(dims=['x'], values=[True, True])},
        ),
    )
    # Assignment replaces data
    assert not sc.identical(v, sc.array(dims=['x'], values=[11, 22], unit='m'))
    assert sc.identical(da.data, sc.array(dims=['x'], values=[11, 22], unit='m'))
    assert sc.identical(c, sc.array(dims=['x'], values=[-1, -2], unit='J'))
    assert sc.identical(a, sc.array(dims=['x'], values=[-100, -200]))
    assert sc.identical(m, sc.array(dims=['x'], values=[False, True]))


def test_own_darr_get():
    # Data and metadata are shared.
    da = make_data_array()[0]
    v = da.data
    c = da.coords['x']
    a = da.attrs['a']
    m = da.masks['m']
    da['x', 0] = -10
    da.data['x', 1] = -20
    da.coords['x']['x', 0] = -1
    da.attrs['a']['x', 0] = -100
    da.masks['m']['x', 0] = False
    c['x', 1] = -2
    a['x', 1] = -200
    m['x', 1] = True
    da.unit = 'kg'
    da.coords['x'].unit = 'J'
    assert sc.identical(
        da,
        sc.DataArray(
            sc.array(dims=['x'], values=[-10, -20], unit='kg'),
            coords={'x': sc.array(dims=['x'], values=[-1, -2], unit='J')},
            attrs={'a': sc.array(dims=['x'], values=[-100, -200])},
            masks={'m': sc.array(dims=['x'], values=[False, True])},
        ),
    )
    assert sc.identical(v, sc.array(dims=['x'], values=[-10, -20], unit='kg'))
    assert sc.identical(c, sc.array(dims=['x'], values=[-1, -2], unit='J'))
    assert sc.identical(a, sc.array(dims=['x'], values=[-100, -200]))
    assert sc.identical(m, sc.array(dims=['x'], values=[False, True]))

    # Assignments overwrite data but not coords.
    da.data = sc.array(dims=['x'], values=[11, 22], unit='m')
    da.coords['x'] = sc.array(dims=['x'], values=[3, 4], unit='s')
    da.attrs['a'] = sc.array(dims=['x'], values=[300, 400])
    da.masks['m'] = sc.array(dims=['x'], values=[True, True])
    assert sc.identical(
        da,
        sc.DataArray(
            sc.array(dims=['x'], values=[11, 22], unit='m'),
            coords={'x': sc.array(dims=['x'], values=[3, 4], unit='s')},
            attrs={'a': sc.array(dims=['x'], values=[300, 400])},
            masks={'m': sc.array(dims=['x'], values=[True, True])},
        ),
    )
    # Assignment replaces data
    assert not sc.identical(v, sc.array(dims=['x'], values=[11, 22], unit='m'))
    assert sc.identical(da.data, sc.array(dims=['x'], values=[11, 22], unit='m'))
    assert sc.identical(c, sc.array(dims=['x'], values=[-1, -2], unit='J'))
    assert sc.identical(a, sc.array(dims=['x'], values=[-100, -200]))
    assert sc.identical(m, sc.array(dims=['x'], values=[False, True]))


def test_own_darr_get_meta():
    # Data and metadata are shared.
    da = make_data_array()[0]
    del da.masks['m']  # not accessible through .meta and tested elsewhere
    v = da.data
    c = da.meta['x']
    a = da.meta['a']
    da['x', 0] = -10
    da.data['x', 1] = -20
    da.coords['x']['x', 0] = -1
    da.attrs['a']['x', 0] = -100
    c['x', 1] = -2
    a['x', 1] = -200
    da.unit = 'kg'
    da.coords['x'].unit = 'J'
    assert sc.identical(
        da,
        sc.DataArray(
            sc.array(dims=['x'], values=[-10, -20], unit='kg'),
            coords={'x': sc.array(dims=['x'], values=[-1, -2], unit='J')},
            attrs={'a': sc.array(dims=['x'], values=[-100, -200])},
        ),
    )
    assert sc.identical(v, sc.array(dims=['x'], values=[-10, -20], unit='kg'))
    assert sc.identical(c, sc.array(dims=['x'], values=[-1, -2], unit='J'))
    assert sc.identical(a, sc.array(dims=['x'], values=[-100, -200]))

    # Assignments overwrite data but not coords.
    da.data = sc.array(dims=['x'], values=[11, 22], unit='m')
    da.coords['x'] = sc.array(dims=['x'], values=[3, 4], unit='s')
    da.attrs['a'] = sc.array(dims=['x'], values=[300, 400])
    assert sc.identical(
        da,
        sc.DataArray(
            sc.array(dims=['x'], values=[11, 22], unit='m'),
            coords={'x': sc.array(dims=['x'], values=[3, 4], unit='s')},
            attrs={'a': sc.array(dims=['x'], values=[300, 400])},
        ),
    )
    # Assignment replaces data
    assert not sc.identical(v, sc.array(dims=['x'], values=[11, 22], unit='m'))
    assert sc.identical(da.data, sc.array(dims=['x'], values=[11, 22], unit='m'))
    assert sc.identical(c, sc.array(dims=['x'], values=[-1, -2], unit='J'))
    assert sc.identical(a, sc.array(dims=['x'], values=[-100, -200]))


def test_own_darr_copy():
    # Depth of copy can be controlled.
    da, _, c, a, m = make_data_array()
    da_copy = copy(da)
    da_deepcopy = deepcopy(da)
    da_methcopy = da.copy(deep=False)
    da_methdeepcopy = da.copy(deep=True)
    da['x', 0] = -10
    da.data['x', 1] = -20
    da.coords['x']['x', 0] = -1
    da.attrs['a']['x', 0] = -100
    da.masks['m']['x', 0] = False
    c['x', 1] = -2
    a['x', 1] = -200
    m['x', 1] = True
    da.unit = 'kg'
    da.coords['x'].unit = 'J'

    modified = sc.DataArray(
        sc.array(dims=['x'], values=[-10, -20], unit='kg'),
        coords={'x': sc.array(dims=['x'], values=[-1, -2], unit='J')},
        attrs={'a': sc.array(dims=['x'], values=[-100, -200])},
        masks={'m': sc.array(dims=['x'], values=[False, True])},
    )
    assert sc.identical(da, modified)
    assert sc.identical(da_copy, modified)
    assert sc.identical(da_deepcopy, make_data_array()[0])
    assert sc.identical(da_methcopy, modified)
    assert sc.identical(da_methdeepcopy, make_data_array()[0])


@pytest.mark.parametrize(
    'data_array_wrapper',
    [lambda k, v: {k: v}, lambda k, v: {k: v}.items(), lambda k, v: sc.Dataset({k: v})],
    ids=['dict', 'iterator', 'Dataset'],
)
def test_own_dset_init(data_array_wrapper):
    da, *_ = make_data_array()
    dset = sc.Dataset(data_array_wrapper('da1', da))

    dset['da1']['x', 0] = -10
    dset['da1'].attrs['a']['x', 0] = -100
    dset['da1'].masks['m']['x', 0] = False
    da['x', 1] = -20
    da.coords['x']['x', 1] = -2
    da.attrs['a']['x', 1] = -200
    da.masks['m']['x', 1] = True
    dset['da1'].unit = 'kg'

    expected = sc.DataArray(
        sc.array(dims=['x'], values=[-10, -20], unit='kg'),
        coords={'x': sc.array(dims=['x'], values=[1, -2], unit='s')},
        attrs={'a': sc.array(dims=['x'], values=[-100, -200])},
        masks={'m': sc.array(dims=['x'], values=[False, True])},
    )
    assert sc.identical(dset, sc.Dataset(data={'da1': expected}))
    assert sc.identical(da, expected)


def test_own_dset_set_access_through_dataarray():
    # The DataArray is shared.
    da, *_ = make_data_array()
    dset = sc.Dataset({'da1': da})

    dset['da1']['x', 0] = -10
    dset['da1'].attrs['a']['x', 0] = -100
    dset['da1'].masks['m']['x', 0] = False
    da['x', 1] = -20
    da.coords['x']['x', 1] = -2
    da.attrs['a']['x', 1] = -200
    da.masks['m']['x', 1] = True
    dset['da1'].unit = 'kg'

    expected = sc.DataArray(
        sc.array(dims=['x'], values=[-10, -20], unit='kg'),
        coords={'x': sc.array(dims=['x'], values=[1, -2], unit='s')},
        attrs={'a': sc.array(dims=['x'], values=[-100, -200])},
        masks={'m': sc.array(dims=['x'], values=[False, True])},
    )
    assert sc.identical(dset, sc.Dataset(data={'da1': expected}))
    assert sc.identical(da, expected)


def test_own_dset_set_access_through_scalar_slice():
    # The DataArray is shared.
    da, *_ = make_data_array()
    dset = sc.Dataset({'da1': da})

    dset['x', 0]['da1'].value = -10
    dset['x', 0]['da1'].attrs['a'].value = -100
    dset['x', 0]['da1'].masks['m'].value = False
    with pytest.raises(sc.VariableError):
        dset['x', 0]['da1'].coords['x'].value = -1
    with pytest.raises(sc.UnitError):
        dset['x', 0]['da1'].unit = 's'

    expected = sc.DataArray(
        sc.array(dims=['x'], values=[-10, 20], unit='m'),
        coords={'x': sc.array(dims=['x'], values=[1, 2], unit='s')},
        attrs={'a': sc.array(dims=['x'], values=[-100, 200])},
        masks={'m': sc.array(dims=['x'], values=[False, False])},
    )
    assert sc.identical(dset, sc.Dataset(data={'da1': expected}))
    assert sc.identical(da, expected)


def test_own_dset_set_access_through_range_slice():
    # The DataArray is shared.
    da, *_ = make_data_array()
    dset = sc.Dataset({'da1': da})

    dset['x', :]['da1']['x', 0] = -10
    dset['x', :]['da1'].attrs['a']['x', 0] = -100
    dset['x', :]['da1'].masks['m']['x', False] = False
    dset['x', :]['da1'].unit = 'kg'

    expected = sc.DataArray(
        sc.array(dims=['x'], values=[-10, 20], unit='kg'),
        coords={'x': sc.array(dims=['x'], values=[1, 2], unit='s')},
        attrs={'a': sc.array(dims=['x'], values=[-100, 200])},
        masks={'m': sc.array(dims=['x'], values=[False, False])},
    )
    assert sc.identical(dset, sc.Dataset(data={'da1': expected}))
    assert sc.identical(da, expected)


def test_own_dset_set_access_through_coords():
    # The DataArray is shared.
    da, *_ = make_data_array()
    dset = sc.Dataset({'da1': da})
    dset.coords['x']['x', 0] = -1

    expected, *_ = make_data_array()
    expected.coords['x']['x', 0] = -1
    assert sc.identical(dset, sc.Dataset(data={'da1': expected}))
    assert sc.identical(da, expected)


def test_own_dset_set_access_through_range_slice_coords():
    # The DataArray is shared.
    da, *_ = make_data_array()
    dset = sc.Dataset({'da1': da})
    dset['x', :]['da1']['x', 0] = -10
    dset['x', :].coords['x']['x', 0] = -1

    expected, *_ = make_data_array()
    expected['x', 0] = -10
    expected.coords['x']['x', 0] = -1
    assert sc.identical(dset, sc.Dataset(data={'da1': expected}))
    assert sc.identical(da, expected)
