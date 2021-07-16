# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
import scipp as sc


def _make_xy():
    Nx = 40
    Ny = 60
    return sc.DataArray(data=sc.ones(dims=['x', 'y'], shape=[Nx, Ny]),
                        coords={
                            'x':
                            sc.linspace(dim='x',
                                        unit='m',
                                        start=0.1,
                                        stop=0.2,
                                        num=Nx),
                            'y':
                            sc.linspace(dim='y',
                                        unit='m',
                                        start=0.1,
                                        stop=0.3,
                                        num=Ny),
                        })


def test_rename_2_steps():
    original = _make_xy()
    da = original.copy().transform_coords(['y3'],
                                          tree={
                                              'y4': 'y',
                                              'y3': 'y2',
                                              'y2': 'y'
                                          })
    assert da.dims == ['x', 'y3']
    original.rename_dims({'y': 'y3'})
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert sc.identical(da.attrs['y2'], original.coords['y'])
    assert sc.identical(da.attrs['y'], original.coords['y'])


def test_rename_multi_output():
    original = _make_xy()
    da = original.copy().transform_coords(['y3', 'y4'],
                                          tree={
                                              'y4': 'y',
                                              'y3': 'y2',
                                              'y2': 'y'
                                          })
    assert da.dims == ['x', 'y']  # y4 also depends on y so dim not renamed
    assert sc.identical(da.coords['y4'], original.coords['y'])
    assert sc.identical(da.coords['y3'], original.coords['y'])
    assert sc.identical(da.attrs['y2'], original.coords['y'])
    assert sc.identical(da.attrs['y'], original.coords['y'])


def ab(*, a, b):
    return a + b


def bc(*, b, c):
    return b + c


def test_dim_rename_split_dim_coord():
    var = sc.arange(dim='b', start=0, stop=4)
    original = sc.DataArray(data=var, coords={'a': var, 'b': var, 'c': var})
    # Only b is dimension-coord, but it is split => no rename of b
    da = original.transform_coords(['ab', 'bc'], tree={'ab': ab, 'bc': bc})
    assert da.dims == ['b']


def test_dim_rename_merge_two_dim_coords():
    a = sc.arange(dim='a', start=0, stop=4)
    b = sc.arange(dim='b', start=0, stop=4)
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    # ab depends on two dimension coords => no rename of a
    da = original.transform_coords(['ab'], tree={'ab': ab})
    assert da.dims == ['a', 'b']
    # Split combined with merge: Ensure the b2 does not cause rename of b
    # which in turn would enable rename of a to ab
    da = original.transform_coords(['ab', 'b2'], tree={'ab': ab, 'b2': 'b'})
    assert da.dims == ['a', 'b']
    da = original.transform_coords(['b2', 'ab'], tree={'ab': ab, 'b2': 'b'})
    assert da.dims == ['a', 'b']


def test_dim_rename_multi_level_merge():
    a = sc.arange(dim='a', start=0, stop=4)
    b = sc.arange(dim='b', start=0, stop=4)
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    da = original.transform_coords(['bc', 'a2'],
                                   tree={
                                       'bc': bc,
                                       'c': 'a',
                                       'a2': 'a'
                                   })
    # a2 prevents conversion of a to c
    # => c is not a dimension coord
    # => bc depends on single dimension coord
    # => rename b to bc
    assert da.dims == ['a', 'bc']


def test_dim_rename_multi_level_merge_multi_output():
    def split_a(*, a):
        return {'a2': a, 'c': a}

    a = sc.arange(dim='a', start=0, stop=4)
    b = sc.arange(dim='b', start=0, stop=4)
    original = sc.DataArray(data=a + b, coords={'a': a, 'b': b})
    da = original.transform_coords(['bc'], tree={'bc': bc, 'c': split_a})
    # Similar to test_dim_rename_multi_level_merge above, but now an implicit
    # intermediate result prevents conversion of a to c
    assert da.dims == ['a', 'bc']
