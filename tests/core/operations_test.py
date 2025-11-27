# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 Scipp contributors (https://github.com/scipp)
import pytest

import scipp as sc


def test_where_with_v_v_v() -> None:
    condition = sc.array(dims=['a'], values=[True, False, False])
    x = sc.array(dims=['a'], values=[1, 2, 3], unit='m')
    y = sc.array(dims=['a'], values=[-13, -11, -12], unit='m')
    result = sc.where(condition, x, y)
    expected = sc.array(dims=['a'], values=[1, -11, -12], unit='m')
    assert sc.identical(result, expected)


def test_where_with_da_v_v() -> None:
    condition = sc.DataArray(
        sc.array(dims=['a'], values=[True, False, False]),
        coords={'a': sc.arange('a', 3), 'b': sc.arange('a', 4)},
    )
    x = sc.array(dims=['a'], values=[1, 2, 3], unit='m')
    y = sc.array(dims=['a'], values=[-13, -11, -12], unit='m')
    result = sc.where(condition, x, y)
    expected = sc.array(dims=['a'], values=[1, -11, -12], unit='m')
    assert sc.identical(result, expected)


def test_where_with_da_da_v() -> None:
    condition = sc.DataArray(
        sc.array(dims=['a'], values=[True, False, False]),
        coords={'a': sc.arange('a', 3), 'b': sc.arange('a', 4)},
    )
    x = sc.DataArray(
        sc.array(dims=['a'], values=[1, 2, 3], unit='m'),
        coords={'a': sc.arange('a', 3)},
    )
    y = sc.array(dims=['a'], values=[-13, -11, -12], unit='m')
    result = sc.where(condition, x, y)
    expected = sc.DataArray(
        sc.array(dims=['a'], values=[1, -11, -12], unit='m'), coords=condition.coords
    )
    assert sc.identical(result, expected)


def test_where_with_da_v_da() -> None:
    condition = sc.DataArray(
        sc.array(dims=['a'], values=[True, False, False]),
        coords={'a': sc.arange('a', 3), 'b': sc.arange('a', 4)},
    )
    x = sc.array(dims=['a'], values=[1, 2, 3], unit='m')
    y = sc.DataArray(
        sc.array(dims=['a'], values=[-13, -11, -12], unit='m'),
        coords={'a': sc.arange('a', 3)},
    )
    result = sc.where(condition, x, y)
    expected = sc.DataArray(
        sc.array(dims=['a'], values=[1, -11, -12], unit='m'), coords=condition.coords
    )
    assert sc.identical(result, expected)


def test_where_with_da_da_da() -> None:
    condition = sc.DataArray(
        sc.array(dims=['a'], values=[True, False, False]),
        coords={'a': sc.arange('a', 3), 'b': sc.arange('a', 4)},
    )
    x = sc.DataArray(
        sc.array(dims=['a'], values=[1, 2, 3], unit='m'),
        coords={'a': sc.arange('a', 3)},
    )
    y = sc.DataArray(
        sc.array(dims=['a'], values=[-13, -11, -12], unit='m'),
        coords={'a': sc.arange('a', 3)},
    )
    result = sc.where(condition, x, y)
    expected = sc.DataArray(
        sc.array(dims=['a'], values=[1, -11, -12], unit='m'), coords=condition.coords
    )
    assert sc.identical(result, expected)


def test_where_with_v_da_da() -> None:
    condition = sc.array(dims=['a'], values=[True, False, False])
    x = sc.DataArray(
        sc.array(dims=['a'], values=[1, 2, 3], unit='m'),
        coords={'a': sc.arange('a', 3)},
    )
    y = sc.DataArray(
        sc.array(dims=['a'], values=[-13, -11, -12], unit='m'),
        coords={'a': sc.arange('a', 3)},
    )
    result = sc.where(condition, x, y)
    expected = sc.DataArray(
        sc.array(dims=['a'], values=[1, -11, -12], unit='m'), coords=x.coords
    )
    assert sc.identical(result, expected)


def test_where_condition_coords_must_be_compatible_with_others() -> None:
    condition = sc.DataArray(
        sc.array(dims=['a'], values=[True, False, False]),
        coords={'a': -sc.arange('a', 3)},
    )
    x = sc.DataArray(
        sc.array(dims=['a'], values=[1, 2, 3], unit='m'),
        coords={'a': sc.arange('a', 3), 'b': sc.arange('a', 4)},
    )
    y = sc.DataArray(
        sc.array(dims=['a'], values=[-13, -11, -12], unit='m'),
        coords={'a': sc.arange('a', 3), 'b': sc.arange('a', 4)},
    )

    with pytest.raises(sc.DatasetError, match="Mismatch in coordinate 'a'"):
        sc.where(condition, x, y)


def test_where_value_coords_must_match() -> None:
    condition = sc.array(dims=['a'], values=[True, False, False])
    x = sc.DataArray(
        sc.array(dims=['a'], values=[1, 2, 3], unit='m'),
        coords={'a': sc.arange('a', 3)},
    )
    y = sc.DataArray(
        sc.array(dims=['a'], values=[-13, -11, -12], unit='m'),
        coords={'a': -sc.arange('a', 3)},
    )

    # differing values
    with pytest.raises(
        sc.DatasetError, match="Expected coords of x and y to match in 'where'"
    ):
        sc.where(condition, x, y)

    # alignment differs
    y.coords['a'] = x.coords['a']
    x.coords.set_aligned('a', False)
    with pytest.raises(sc.DatasetError):
        sc.where(condition, x, y)

    # extra coord
    y.coords['b'] = sc.arange('a', 3)
    with pytest.raises(sc.DatasetError):
        sc.where(condition, x, y)


@pytest.mark.parametrize('argument', ['condition', 'x', 'y'])
def test_where_does_not_allow_masks(argument: str) -> None:
    condition = sc.DataArray(
        sc.array(dims=['a'], values=[True, False, False]),
        coords={'a': sc.arange('a', 3)},
    )
    x = sc.DataArray(
        sc.array(dims=['a'], values=[1, 2, 3], unit='m'),
        coords={'a': sc.arange('a', 3)},
    )
    y = sc.DataArray(
        sc.array(dims=['a'], values=[-13, -11, -12], unit='m'),
        coords={'a': sc.arange('a', 3)},
    )

    locals()[argument].masks['m'] = sc.array(dims=['a'], values=[True, False, True])
    with pytest.raises(ValueError, match="must not have masks"):
        sc.where(condition, x, y)


def test_clip_with_min_and_max() -> None:
    x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0, 5.0], unit='m')
    result = sc.clip(x, min=sc.scalar(2.0, unit='m'), max=sc.scalar(4.0, unit='m'))
    expected = sc.array(dims=['x'], values=[2.0, 2.0, 3.0, 4.0, 4.0], unit='m')
    assert sc.identical(result, expected)


def test_clip_with_only_min() -> None:
    x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0, 5.0], unit='m')
    result = sc.clip(x, min=sc.scalar(2.0, unit='m'))
    expected = sc.array(dims=['x'], values=[2.0, 2.0, 3.0, 4.0, 5.0], unit='m')
    assert sc.identical(result, expected)


def test_clip_with_only_max() -> None:
    x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0, 5.0], unit='m')
    result = sc.clip(x, max=sc.scalar(4.0, unit='m'))
    expected = sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0, 4.0], unit='m')
    assert sc.identical(result, expected)


def test_clip_raises_when_both_none() -> None:
    x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0, 5.0])
    with pytest.raises(
        ValueError, match="At least one of 'min' or 'max' must be provided"
    ):
        sc.clip(x)


def test_clip_with_data_array() -> None:
    x = sc.DataArray(
        sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0, 5.0], unit='m'),
        coords={'x': sc.arange('x', 5)},
    )
    result = sc.clip(x, min=sc.scalar(2.0, unit='m'), max=sc.scalar(4.0, unit='m'))
    expected = sc.DataArray(
        sc.array(dims=['x'], values=[2.0, 2.0, 3.0, 4.0, 4.0], unit='m'),
        coords={'x': sc.arange('x', 5)},
    )
    assert sc.identical(result, expected)


def test_clip_preserves_variances() -> None:
    x = sc.array(
        dims=['x'],
        values=[1.0, 2.0, 3.0, 4.0, 5.0],
        variances=[0.1, 0.2, 0.3, 0.4, 0.5],
        unit='m',
    )
    result = sc.clip(x, min=sc.scalar(2.0, unit='m'), max=sc.scalar(4.0, unit='m'))
    # Clipped values get variance from the scalar (which has no variance, so 0)
    # Non-clipped values keep their original variance
    expected = sc.array(
        dims=['x'],
        values=[2.0, 2.0, 3.0, 4.0, 4.0],
        variances=[0.0, 0.2, 0.3, 0.4, 0.0],
        unit='m',
    )
    assert sc.identical(result, expected)
