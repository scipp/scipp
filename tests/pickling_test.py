# ruff: noqa: S301

try:
    import _pickle as pickle
except ImportError:
    import pickle  # type: ignore[no-redef]

import multiprocessing.dummy
import re
import subprocess
import sys
from collections.abc import Callable, Sequence
from datetime import datetime
from functools import partial
from operator import eq
from pathlib import Path
from textwrap import dedent
from typing import Any, cast

import numpy as np
import numpy.typing as npt
import pytest
from scipy.spatial.transform import Rotation

import scipp as sc
from scipp import spatial

_64_BIT = sys.maxsize > (2 << 32)
_DEFAULT_INT_TYPE = str(np.dtype(int))
_DEFAULT_FLOAT_TYPE = str(np.dtype(float))


def _roundtrip[T](
    obj: T,
    cmp: Callable[[T, T], bool] = eq,
    *,
    cross_proc: bool = False,
    tmpdir: Path | None = None,
) -> None:
    """
    Test round-tripping ``obj`` via :mod:`pickle`.

    Tests:
    - Round-tripping inside the process
    - Optionally (if ``cross_proc``):
      - Unpickling in a subprocess
      - Re-pickling in said subprocess,
      - Unpickling said pickle in this main process, and
      - Comparison with the ``obj`` via ``cmp``.
    """

    # Test in-process roundtripping
    serialized = pickle.dumps(obj)
    deserialized = pickle.loads(serialized)
    assert deserialized is not obj
    assert cmp(obj, deserialized)

    cls = type(obj)
    assert cls is getattr(sc, cls.__name__)

    if not cross_proc:
        return
    assert tmpdir is not None

    # Test that other processes using the same Python can also
    # (de-)serialize it (i.e. the hooks are set up correctly on
    # importing `scipp`)
    src = tmpdir / 'src.pkl'
    dest = tmpdir / 'dest.pkl'
    src.write_bytes(serialized)
    python_script = dedent(f"""
    try:
        import _pickle as pickle
    except ImportError:
        import pickle
    import sys


    obj, src, dest = sys.argv[-3:]
    with open(src, mode='rb') as fobj:
        deserialized = pickle.load(fobj)

    # Defer import of `scipp` to see if the our hooks are set up
    # automatically
    from scipp import {cls.__name__}
    assert isinstance(deserialized, {cls.__name__})

    # Re-pickle the loaded object so that the calling process can
    # reuse it for whatever (e.g. comparison)
    with open(dest, mode='wb') as fobj:
        pickle.dump(deserialized, fobj)
    """)
    cmd = [sys.executable, '-c', python_script, str(obj), str(src), str(dest)]
    subprocess.run(cmd, check=True)  # noqa: S603

    # Check the cross-process-round-tripped object
    with open(dest, mode='rb') as fobj:
        deserialized = pickle.load(fobj)
    assert cmp(obj, deserialized)


def _cmp_units(u1: sc.Unit, u2: sc.Unit) -> bool:
    if u1 == u2:
        return True
    # Assure the typechecker that the mults are numbers or it will
    # complain about the `isnan()` calls
    mult1 = cast(float, (d1 := u1.to_dict()).pop('multiplier', 1))
    mult2 = cast(float, (d2 := u2.to_dict()).pop('multiplier', 1))
    if d1 != d2:
        return False
    # Coerce to Python bools to appease the typechecker
    return bool(np.isnan(mult1) and np.isnan(mult2))


def _cmp_vars(v1: sc.Variable, v2: sc.Variable) -> bool:
    """
    :func:`scipp.identical()` doesn't check user-assigned attributes, so
    do it here.

    Example
    -------
    >>> from operator import is_
    >>> from scipp import identical, scalar

    >>> v1 = scalar(5)
    >>> v2 = v1.copy()
    >>> identical(v1, v2), is_(v1, v2), _cmp_vars(v1, v2)
    (True, False, True)

    >>> v1.foo = 6
    >>> v1.foo
    6
    >>> identical(v1, v2), is_(v1, v2), _cmp_vars(v1, v2)
    (True, False, False)
    """
    return vars(v1) == vars(v2) and sc.identical(v1, v2)


def _cmp_arrays(a1: sc.DataArray, a2: sc.DataArray) -> bool:
    """
    :func:`scipp.identical()` doesn't check the array names, so do it
    here.

    Example
    -------
    >>> from operator import is_
    >>> from scipp import DataArray, array, identical

    >>> a1 = DataArray(data=array(values=[5], dims=['_']), name='foo')
    >>> a2 = a1.copy()
    >>> identical(a1, a2), is_(a1, a2), _cmp_arrays(a1, a2)
    (True, False, True)

    >>> a2.name = 'bar'
    >>> a2.name
    'bar'
    >>> identical(a1, a2), is_(a1, a2), _cmp_arrays(a1, a2)
    (True, False, False)
    """
    return a1.name == a2.name and sc.identical(a1, a2)


def _cmp_datasets(ds1: sc.Dataset, ds2: sc.Dataset) -> bool:
    if (keys := set(ds1)) != set(ds2):
        return False
    return all(sc.identical(ds1[key], ds2[key]) for key in keys)


@pytest.mark.parametrize(
    ('expected', 'dtypes'),
    # XXX: will there ever possibly be `DType` objects that don't
    # correspond to one of the class members?
    [
        ('int64', ['int64', np.int64]),
        ('float64', ['float64', np.float64]),
        ('int32', ['int32', np.int32]),
        ('float32', ['float32', np.float32]),
        ('int64' if _64_BIT else 'int32', [int]),
        ('float64' if _64_BIT else 'float32', [float]),
        ('bool', ['bool', np.bool_, bool]),
        ('string', ['string', np.str_, str]),
        ('datetime64', ['datetime64', np.datetime64]),
        ('uint64', ['uint64', np.uint64]),
    ]
    + [
        (dtype, None)
        for dtype in [
            'DataArray',
            'Dataset',
            'PyObject',
            'Variable',
            'DataArrayView',
            'DatasetView',
            'VariableView',
            'affine_transform3',
            'linear_transform3',
            'rotation3',
            'translation3',
            'vector3',
        ]
    ],
)
def test_pickling_dtypes(expected: str, dtypes: list[Any] | None) -> None:
    """
    Test that :class:`DType`s can be round-tripped in-process.
    """
    # Set up the dtype
    if dtypes is None:
        dtypes = [expected]
    for dtype in dtypes:
        try:
            dt = sc.DType(dtype)
        except TypeError:
            assert isinstance(dtype, str)
            dt = getattr(sc.DType, dtype)
        else:
            assert dt == getattr(sc.DType, expected)
        # Actual comparison
        _roundtrip(dt)


@pytest.mark.parametrize(
    'unit',
    [
        'm',  # Simple unit
        '3.14159 m',  # Unit + multiplier
        'J*m*kg**-1',  # Composite unit
        '6.022E23',  # Dimensionless
        # Edge cases: infinities and nans
        'inf N*m**-2',
        '-inf s',
        'nan m',
    ],
)
def test_pickling_units(unit: str) -> None:
    """
    Test that :class:`Unit`s can be round-tripped in-process.
    """
    _roundtrip(sc.Unit(unit), _cmp_units)


def _get_var_with_attrs() -> sc.Variable:
    inner_var = sc.arange('x', 0, 10)
    var, dtype, shape = sc.scalar(inner_var), 'Variable', ()
    var.foo, var.bar, var.baz = None, 1, []  # type: ignore[attr-defined]
    assert vars(var) == {'foo': None, 'bar': 1, 'baz': []}
    return var


def _get_translation() -> sc.Variable:
    return spatial.translation(value=np.array([3.0, 2.0, 1.0]), unit='m')


def _get_rotation() -> sc.Variable:
    return spatial.rotation(
        value=Rotation.from_rotvec([1.0, 2.0, 3.0]).as_quat(),
    )


@pytest.mark.parametrize(
    ('var', 'dtype', 'shape'),
    [
        # Scalars
        pytest.param(sc.scalar(5), _DEFAULT_INT_TYPE, (), id='scalar-int'),
        pytest.param(sc.scalar('foo'), 'string', (), id='scalar-str'),
        pytest.param(
            sc.scalar(np.datetime64(datetime.now().isoformat())),
            'datetime64',
            (),
            id='scalar-datetime',
        ),
        pytest.param(
            sc.scalar('foo', dtype=object),
            'PyObject',
            (),
            id='scalar-object',
        ),
        pytest.param(
            sc.scalar(5.0, variance=0.25),
            _DEFAULT_FLOAT_TYPE,
            (),
            id='scalar-float-with-variance',
        ),
        pytest.param(
            sc.vector([1.0, 2.0, 3.0], unit='m'),
            'vector3',
            (),
            id='scalar-vector-with-unit',
        ),
        pytest.param(
            _get_var_with_attrs(),
            'Variable',
            (),
            id='scalar-variable-with-attributes',
        ),
        # Scalars (linalg types)
        pytest.param(
            _get_translation(),
            'translation3',
            (),
            id='scalar-translation',
        ),
        pytest.param(
            _get_rotation(),
            'rotation3',
            (),
            id='scalar-rotation',
        ),
        pytest.param(
            _get_translation() * _get_rotation(),
            'affine_transform3',
            (),
            id='scalar-affine-transformation',
        ),
    ]
    # N-D arrays
    + [
        pytest.param(
            sc.Variable(
                values=np.random.random(shape := (10, 10)),
                variances=np.random.random(shape),
                dtype=sc.DType.float64,
                dims=['x', 'y'],
            ),
            'float64',
            shape,
            id='float64s-2D-with-variances',
        ),
    ]
    + [
        pytest.param(
            sc.Variable(
                values=np.random.choice(range(10), (shape := (10,) * ndim)),
                dims=['x', 'y', 'z'][:ndim],
            ),
            _DEFAULT_INT_TYPE,
            shape,
            id=f'ints-{ndim}D',
        )
        for ndim in [1, 2, 3]
    ]
    + [
        pytest.param(
            sc.Variable(
                values=np.random.choice(range(10), (size := 10) ** ndim),
                dims=['_'],
            ).fold(
                '_',
                shape=(shape := (size,) * ndim),
                dims=['x', 'y', 'z'][:ndim],
            ),
            _DEFAULT_INT_TYPE,
            shape,
            id=f'ints-{ndim}D-folded',
        )
        for ndim in [1, 2, 3]
    ]
    + [
        pytest.param(
            sc.array(
                values=np.random.random(shape := (10,) * ndim),
                dims=['x', 'y', 'z'][:ndim],
                dtype=getattr(sc.DType, dtype),
            ),
            dtype,
            shape,
            id=f'{dtype}s-{ndim}D',
        )
        for dtype, ndim in [('float32', 1), ('float64', 1), ('float64', 3)]
    ]
    # N-D arrays (linalg types)
    + [
        pytest.param(
            constructor(
                values=np.random.random((shape := (10,) * ndim) + item_shape),
                dims=['x', 'y', 'z'][:ndim],
            ),
            dtype,
            shape,
            id=f'{name}-{ndim}D',
        )
        for name, dtype, item_shape, constructor in [
            ('vecs', 'vector3', (3,), spatial.vectors),
            ('translations', 'translation3', (3,), spatial.translations),
            ('lintr', 'linear_transform3', (3, 3), spatial.linear_transforms),
            ('afftr', 'affine_transform3', (4, 4), spatial.affine_transforms),
        ]
        for ndim in [1, 2, 3]
    ]
    + [
        pytest.param(
            spatial.rotations(
                values=Rotation.from_rotvec(
                    np.random.random((shape := (10,) * ndim) + (3,)),
                ).as_quat(),
                dims=['x', 'y', 'z'][:ndim],
            ),
            'rotation3',
            shape,
            id=f'rotations-{ndim}D',
        )
        for ndim in [1, 2, 3]
    ]
    # N-D arrays (non-native types)
    + [
        pytest.param(
            sc.array(
                values=np.random.choice(
                    ['foo', 'bar', 'baz'],
                    (size := 10) ** ndim,
                ).astype(hint),
                dims=['_'],
            ).fold(
                '_',
                shape=(shape := (size,) * ndim),
                dims=['x', 'y', 'z'][:ndim],
            ),
            dtype,
            shape,
            id=f'{name}s-{ndim}D',
        )
        for name, dtype, hint in [
            ('str', 'string', str),
            ('obj', 'PyObject', object),
        ]
        for ndim in [1, 2, 3]
    ],
)
def test_pickling_unbinned_variables(
    var: sc.Variable,
    dtype: str,
    shape: tuple[int, ...],
) -> None:
    """
    With handpicked examples, test that un-binned :class:`Variable`s can
    be round-tripped in-process.
    """
    assert var.dtype == getattr(sc.DType, dtype)
    assert var.shape == shape
    _roundtrip(var, _cmp_vars)


@pytest.mark.parametrize(
    'shape',
    [
        '0',
        '0,1',
        '0,2',
        '1,0',
        '2,0',
        '0,0,1',
        '0,0,2',
        '0,1,0',
        '0,2,0',
        '1,0,0',
        '2,0,0',
    ],
)
def test_pickling_empty_variables(shape: str) -> None:
    """
    With handpicked examples, test that empty :class:`Variable`s (i.e.
    those with one or more dimensions being zero-sized) can be
    round-tripped in-process.

    See also
    --------
    tests/compat/dict_test.py::test_empty_variable_to_dict()
    """
    shape_tuple = tuple(int(i) for i in shape.split(','))
    dims = tuple(f'dim{i}' for i, _ in enumerate(shape_tuple))
    var = sc.empty(shape=(0,), dims=('_',))
    var = var.fold('_', shape=shape_tuple, dims=dims)
    assert var.shape == shape_tuple
    _roundtrip(var, _cmp_vars)


@pytest.mark.parametrize(
    'case',
    # Most of the machineries should have been tested to be working via
    # `test_pickling_unbinned_variables()`.
    [
        # Scalars
        'scalar-float',
        'scalar-obj',
        # Empty arrays
        'nothing-2D',
        # Normal arrays
        'int-1D',
        'float-1D-with-name',
        'float-2D-with-mask',
        'vec-3D',
        'string-2D',
        'obj-2D',
        'linear-transform-1D',
    ],
)
def test_pickling_unbinned_data_arrays(case: str) -> None:
    """
    With handpicked examples, test that un-binned :class:`DataArray`s
    can be round-tripped in-process.
    """
    rng = np.random.default_rng(42)
    length = 10
    if case == 'float-1D-with-name':
        da = sc.DataArray(
            data=sc.Variable(
                values=rng.random((length,)),
                dims=['t'],
                unit='J',
            ),
            coords={'t': sc.arange('t', 0, length, unit='s')},
            name='name',
        )
        assert da.name == 'name'
    elif case == 'float-2D-with-mask':
        da = sc.DataArray(
            data=sc.Variable(
                values=rng.random((length, length)),
                dims=['x', 'y'],
                unit='J',
            ),
            coords={xy: sc.arange(xy, 0, length, unit='m') for xy in ['x', 'y']},
            masks={
                'x': sc.Variable(
                    values=rng.choice([True, False], length).astype(bool),
                    dims=['x'],
                ),
            },
        )
        assert da.masks

    elif case.startswith('scalar-'):
        dtype_hint: Any
        value, dtype_hint = {
            'float': (5.0, None),
            'obj': ('some obj', object),
        }[case.partition('-')[-1]]
        da = sc.DataArray(
            data=sc.Variable(values=value, dims=(), dtype=dtype_hint),
            coords={},
        )
        assert not da.shape
        assert not da.coords

    elif case == 'nothing-2D':
        shape: tuple[int, ...] = (0, 0)
        da = sc.DataArray(
            data=sc.Variable(
                values=np.array([], dtype='<U6').reshape(shape),
                dims=['x', 'y'],
                dtype=str,
            ),
            coords={xy: sc.Variable(values=[], dims=[xy]) for xy in ['x', 'y']},
            name='nothing',
        )
        assert da.shape == shape
        assert da.dtype == sc.DType.string
        assert da.sizes == {'x': 0, 'y': 0}

    else:
        assert case.endswith(('-1D', '-2D', '-3D'))
        var_type, _, dim_ = case.rpartition('-')
        ndim = int(dim_.rstrip('D'))
        make_var: Callable[..., sc.Variable]
        shape = (length,) * ndim
        choices: Sequence[Any] | npt.NDArray[Any]
        extra_shape: tuple[int, ...]
        make_var, choices, dtype = {  # type: ignore[assignment]
            'int': (sc.Variable, range(20), sc.DType(np.dtype(int))),
            'vec': (sc.vectors, np.linspace(0, 1), sc.DType.vector3),
            'string': (sc.Variable, ['foo', 'bar', 'baz'], sc.DType.string),
            'obj': (sc.Variable, ['foo', 'bar', 'baz'], sc.DType.PyObject),
            'linear-transform': (
                spatial.linear_transforms,
                np.linspace(0, 10),
                sc.DType.linear_transform3,
            ),
        }[var_type]
        extra_shape = {
            'vec': (3,),
            'linear-transform': (3, 3),
        }.get(var_type, ())

        flat_data = rng.choice(
            choices,
            (np.prod(shape), *extra_shape),
        )
        if var_type == 'obj':  # `numpy.str_` -> `str`
            flat_data = flat_data.astype(object)
        var = make_var(
            values=flat_data,  # type: ignore[arg-type]
            dims=['_'],
        ).fold('_', dims=['x', 'y', 'z'][:ndim], shape=(length,) * ndim)
        da = sc.DataArray(
            data=var,
            coords={
                xyz: sc.arange(xyz, 0, length, unit='m')
                for xyz in ['x', 'y', 'z'][:ndim]
            },
        )
        assert da.shape == shape
        assert da.dtype == dtype

    _roundtrip(da, _cmp_arrays)


@pytest.mark.parametrize(
    'case',
    [
        'implicit-coords-and-arrays',
        'explicit-coords-and-variables',
        'masked-entries',
        'renamed-entries',
        'empty-with-coords',
        'empty-without-coords',
    ],
)
def test_pickling_unbinned_datasets(case: str) -> None:
    """
    With handpicked examples, test that un-binned :class:`Dataset`s
    can be round-tripped in-process.
    """
    rng = np.random.default_rng(42)

    n = 100
    t = sc.linspace(start=0, stop=10, num=n, dim='t', unit='s')
    x = t * sc.Unit('50 m*s**-1')
    foo_var = sc.array(values=rng.random(n), dims=['t'])
    bar_var = sc.array(values=rng.random(n), dims=['t'])

    # Basic cases
    if case == 'implicit-coords-and-arrays':
        foo_da = sc.DataArray(data=foo_var, coords={'t': t})
        bar_da = sc.DataArray(data=bar_var, coords={'x': x})
        ds = sc.Dataset({'foo': foo_da, 'bar': bar_da})
        assert set(ds.coords) == {'x', 't'}
        assert set(ds) == {'foo', 'bar'}
    elif case == 'explicit-coords-and-variables':
        ds = sc.Dataset({'foo': foo_var, 'bar': bar_var}, coords={'x': x, 't': t})
        assert set(ds.coords) == {'x', 't'}
        assert set(ds) == {'foo', 'bar'}

    # Edge case: empty datasets
    elif case in ('empty-with-coords', 'empty-without-coords'):
        if 'without' in case:
            ds = sc.Dataset({})
            assert not ds.coords
        else:
            ds = sc.Dataset({}, coords={'x': x, 't': t})
            assert set(ds.coords) == {'x', 't'}
        assert not len(ds)

    # Misc. cases
    elif case == 'masked-entries':
        foo_da = sc.DataArray(
            data=foo_var,
            coords={'t': t},
            masks={'t': t < sc.scalar(value=5.0, unit='s')},
        )
        ds = sc.Dataset(  # Mixed coords source
            cast(dict[str, Any], {'foo': foo_da, 'bar': bar_var}),
            coords={'x': x},
        )
        assert set(ds.coords) == {'x', 't'}
        assert set(ds) == {'foo', 'bar'}
        mask = ds['foo'].masks['t']
        assert (mask['t', 0].value, mask['t', -1].value) == (True, False)
        assert not ds['bar'].masks
    elif case == 'renamed-entries':
        foo_da = sc.DataArray(
            data=foo_var,
            coords={'t': t},
            name='baz',  # Overwritten
        )
        ds = sc.Dataset(  # Mixed coords source
            cast(dict[str, Any], {'foo': foo_da, 'bar': bar_var}),
            coords={'x': x},
        )
        assert set(ds.coords) == {'x', 't'}
        assert set(ds) == {'foo', 'bar'}
        assert ds['foo'].name == 'foo'
        assert ds['bar'].name == 'bar'

    else:
        raise AssertionError(f'{case=!r}')

    _roundtrip(ds, _cmp_datasets)


@pytest.mark.parametrize('case', ['Variable', 'DataArray', 'Dataset'])
def test_pickling_binned_data(case: str) -> None:
    """
    With handpicked examples, test that binned data can be round-tripped
    in-process.
    """
    times = sc.array(
        values=np.random.default_rng(42).random(100),
        dims=['t'],
        unit='ms',
    )
    events = sc.DataArray(
        data=sc.ones(shape=[100], dims=['t']),
        coords={'t': times},
    )
    binned = events.bin(t=10)

    pickled: sc.Dataset | sc.DataArray | sc.Variable
    cmp: Callable[[Any, Any], bool]
    if case == 'Dataset':
        binned.name = 'data'
        pickled, cmp = sc.Dataset({'data': binned}), _cmp_datasets
    elif case == 'DataArray':
        pickled, cmp = binned, _cmp_arrays
    elif case == 'Variable':
        pickled, cmp = binned.data, _cmp_vars
    else:
        raise AssertionError(f'{case=!r}')

    assert type(pickled), __name__ == case
    assert pickled.is_binned
    _roundtrip(pickled, cast(Callable[..., bool], cmp))


@pytest.mark.parametrize(
    ('obj', 'cmp'),
    [
        pytest.param(sc.DType.PyObject, eq, id='DType'),
        pytest.param(sc.Unit('5 cm'), _cmp_units, id='Unit'),
        pytest.param(
            sc.array(values=[1, 2, 3], dims=['x'], unit='J'),
            _cmp_vars,
            id='Variable',
        ),
        pytest.param(
            sc.array(
                values=np.random.default_rng(42).random(100),
                dims=['x'],
                unit='m',
            ).bin(x=10),
            _cmp_arrays,
            id='Array-binned',
        ),
        pytest.param(
            sc.Dataset(
                {'b': sc.array(values=[True, False, True], dims=['foo'])},
                coords={'foo': sc.array(values=[0.0, 4.0, 9.0], dims=['foo'])},
            ),
            _cmp_datasets,
            id='Dataset',
        ),
    ],
)
def test_cross_process_round_tripping[T](
    tmp_path: Path,
    obj: T,
    cmp: Callable[[T, T], bool],
) -> None:
    """
    Test that various :mod:`scipp` types can be round-tripped
    cross-process via :mod:`pickle`.
    """
    _roundtrip(obj, cmp, tmpdir=tmp_path, cross_proc=True)


@pytest.mark.parametrize(
    'pool',
    [
        pytest.param(multiprocessing.dummy.Pool(), id='dummy'),
        pytest.param(
            multiprocessing.get_context('spawn').Pool(4),
            id='spawn-4',
        ),
    ],
)
def test_multiproc_worker_communication(
    pool: multiprocessing.pool.Pool,
) -> None:
    """
    Integration test: :mod:`scipp` types can be sent to and from
    spawned :mod:`multiprocessing` worker processes.
    """
    base = sc.Dataset(
        {
            'f': sc.array(
                values=np.random.default_rng(42).random(100),
                dims=['x'],
            ),
            'v': sc.array(
                values=np.random.default_rng(42).random((100, 3)),
                dims=['x'],
                dtype=sc.DType.vector3,
            ),
        },
        coords={
            'x': sc.linspace(start=0, stop=1, num=100, dim='x'),
        },
    )
    with pool:
        n = 4
        workloads = [base['x', start::n] for start in range(n)]
        results = pool.map(sc.sum, workloads)
    for workload, result in zip(workloads, results, strict=True):
        assert _cmp_datasets(workload.sum(), result)


def test_unpickling_wrong_scipp_version(tmp_path: Path) -> None:
    """
    Test that unpickling a pickled :mod:`scipp` object from the wrong
    version results in an error.
    """
    version_at_pickling = sc.__version__
    version_at_unpickling = sc.__version__ + '.patch'

    unit = sc.Unit('5 m')
    fname = tmp_path / 'unit.pkl'
    with fname.open(mode='wb') as fobj:
        pickle.dump(unit, fobj)

    with pytest.MonkeyPatch.context() as mp:
        mp.setattr('scipp._pickling._SCIPP_VERSION', version_at_unpickling)
        msg = 'version mismatch: .*' + re.escape(
            f'package version = {version_at_unpickling}, '
            f'pickled version = {version_at_pickling}; '
        )
        with pytest.raises(RuntimeError, match=msg):
            with fname.open(mode='rb') as fobj:
                pickle.load(fobj)
