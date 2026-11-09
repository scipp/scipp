# ruff: noqa: S301

try:
    import _pickle as pickle
except ImportError:
    import pickle  # type: ignore[no-redef]

import re
import subprocess
import sys
from collections.abc import Callable, Sequence
from dataclasses import dataclass
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
from scipp._pickle import VersionError

_64_BIT = sys.maxsize > (2 << 32)
_DEFAULT_INT_TYPE = str(np.dtype(int))
_DEFAULT_FLOAT_TYPE = str(np.dtype(float))


@dataclass(eq=True)
class Person:
    """Dummy hashable class which is resolved to a ``PyObject``."""
    name: str


def _roundtrip[T](
    tmpdir: Path, obj: T, cmp: Callable[[T, T], bool] | bool = True,
) -> None:
    """
    Test round-tripping ``obj`` via :mod:`pickle`.

    Tests:
    - Round-tripping inside the process
    - Unpickling in a subprocess
    - Optionally, re-pickling in said subprocess, unpickling in this
      main process, and comparison with the ``obj`` via ``cmp`` (see
      below).

    Notes
    -----
    - ``cmp=<callable>`` compares the two arguments (``obj`` and the
      roundtripped object) and should return true(y) for success.
    - ``cmp=True`` is an alias for ``cmp=operator.eq``.
    - ``cmp=False`` skips re-pickling in the child process and the
      subsequent comparison of the values, only checking that the child
      can unpickle what this main process has pickled.
    """
    comparator: Callable[[T, T], bool]
    if cmp in (True, False):
        comparator = eq
        cross_proc_roundtrip = cmp
    else:
        comparator, cross_proc_roundtrip = cast(Callable[..., bool], cmp), True

    # Test in-process roundtripping
    serialized = pickle.dumps(obj)
    deserialized = pickle.loads(serialized)
    assert deserialized is not obj
    assert comparator(obj, deserialized)

    cls = type(obj)
    assert cls is getattr(sc, cls.__name__)

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
    """)
    if cross_proc_roundtrip:
        python_script += '\n' + dedent("""
        # Re-pickle the loaded object so that the calling process can
        # reuse it for whatever (e.g. comparison)
        with open(dest, mode='wb') as fobj:
            pickle.dump(deserialized, fobj)
        """)
    cmd = [sys.executable, '-c', python_script, str(obj), str(src), str(dest)]
    subprocess.run(cmd, check=True)  # noqa: S603

    # Check the cross-process-roundtripped object, if necessary
    if comparator is not None:
        with open(dest, mode='rb') as fobj:
            deserialized = pickle.load(fobj)
        assert comparator(obj, deserialized)


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
        (dtype, None) for dtype in [
            'DataArray', 'Dataset', 'PyObject', 'Variable',
            'DataArrayView', 'DatasetView', 'VariableView',
            'affine_transform3', 'linear_transform3',
            'rotation3', 'translation3', 'vector3',
        ]
    ],
)
def test_pickling_dtypes(
    tmp_path: Path, expected: str, dtypes: list[Any] | None,
) -> None:
    """
    Test that :class:`DType`s can be roundtripped in- and cross-process.
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
        _roundtrip(tmp_path, dt)


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
    ]
)
def test_pickling_units(tmp_path: Path, unit: str) -> None:
    """
    Test that :class:`Unit`s can be roundtripped in- and cross-process.
    """
    def cmp_units(u1: sc.Unit, u2: sc.Unit) -> bool:
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

    _roundtrip(tmp_path, sc.Unit(unit), cmp_units)


@pytest.mark.parametrize(
    'case',
    [
        # Scalars
        'scalar-int',
        'scalar-str',
        'scalar-datetime',
        'scalar-object',
        'scalar-float-with-variance',
        'scalar-vector-with-unit',
        # Scalars (linalg types)
        'scalar-translation',
        'scalar-rotation',
        'scalar-linear-transform',
        'scalar-affine-transform',
        # Zero-entry arrays
        'nothing-1D',
        'nothing-2D',
        'nothing-3D',
        # N-D arrays
        'ints-1D',
        'ints-2D',
        'ints-3D',
        'ints-1D-folded',
        'ints-2D-folded',
        'ints-3D-folded',
        'float32s-1D',
        'float64s-1D',
        'float64s-2D-with-variances',
        'float64s-3D',
        'vecs-1D',
        'vecs-2D',
        'vecs-3D',
        'strs-1D',
        'strs-2D-folded',
        'strs-3D-folded',
        'objs-1D',
        'objs-2D-folded',
        'objs-3D-folded',
        # N-D arrays (linalg types)
        'translation-1D',
        'translation-2D',
        'translation-3D',
        'rotation-1D',
        'rotation-2D',
        'rotation-3D',
        'lintr-1D',
        'lintr-2D',
        'lintr-3D',
        'afftr-1D',
        'afftr-2D',
        'afftr-3D',
    ],
)
def test_pickling_unbinned_variables(
    tmp_path: Path, case: str,
) -> None:
    """
    With handpicked examples, test that un-binned :class:`Variable`s can
    be roundtripped in- and cross-process.
    """
    rng = np.random.default_rng(42)
    length = 5  # Size along multidim-array dimensions

    # Scalar test cases
    if case == 'scalar-float-with-variance':
        var = sc.scalar(value=5., variance=1.)
        dtype = _DEFAULT_FLOAT_TYPE
        shape: tuple[int, ...] = ()
    elif case == 'scalar-vector-with-unit':
        var, dtype, shape = sc.vector([1, 2, 3], unit='m'), 'vector3', ()

    elif case in (
        'scalar-translation', 'scalar-rotation',
        'scalar-affine-transform',
    ):
        # FIXME: direct instantiation from a `Sequence[int]` appears in
        # the docs, but doesn't type-check because Python `float`s are
        # neither numpy `float32`s nor `float64`s.
        tr = spatial.translation(value=np.array([3., 2., 1.]), unit='m')
        rot = spatial.rotation(
            value=Rotation.from_rotvec([1., 2., 3.]).as_quat(),
        )
        var, dtype = {
            'translation': (tr, 'translation3'),
            'rotation': (rot, 'rotation3'),
            'affine-transform': (tr * rot, 'affine_transform3'),
        }[case.removeprefix('scalar-')]
        shape = ()
    elif case.startswith('scalar-'):
        value: Any
        value, dtype, val_to_var = {
            'int': (42, _DEFAULT_INT_TYPE, sc.scalar),
            'str': ('foo', 'string', sc.scalar),
            'datetime': (
                np.datetime64('1970-01-01T00:00:00'), 'datetime64', sc.scalar,
            ),
            'object': (Person('someone'), 'PyObject', sc.scalar),
            'linear-transform': (
                [[-2, 1, 1], [1, -2, 1], [1, 1, -2]],
                'linear_transform3',
                spatial.linear_transform,
            ),
        }[case.removeprefix('scalar-')]
        var = val_to_var(value=value)
        shape = ()

    # Zero-entry test cases
    elif case.startswith('nothing-'):
        ndim = int(case.rpartition('-')[-1].rstrip('D'))
        shape = (2,) * (ndim - 1) + (0,)
        data = np.array([], dtype=float).reshape(shape)
        var = sc.array(values=data, dims=['x', 'y', 'z'][:ndim])
        dtype = _DEFAULT_FLOAT_TYPE
        assert var.ndim == ndim

    # N-D array test cases
    elif case == 'float64s-2D-with-variances':
        shape = (10, 10)
        values_2d = rng.random(shape)
        variances_2d = rng.random(shape)
        var = sc.array(
            values=values_2d, variances=variances_2d, dims=['x', 'y'],
        )
        dtype = _DEFAULT_FLOAT_TYPE

    elif case.startswith(('translation-', 'rotation-', 'lintr-', 'afftr-')):
        vartype, _, dims = case.rpartition('-')
        ndim = int(dims.rstrip('D'))
        shape = (length,) * ndim
        values, dtype, vals_to_var = {
            'translation': (
                rng.random((*shape, 3)), 'translation3', spatial.translations,
            ),
            'rotation': (
                Rotation.from_rotvec(rng.random((*shape, 3))).as_quat(),
                'rotation3',
                spatial.rotations,
            ),
            'lintr': (
                rng.random((*shape, 3, 3)),
                'linear_transform3',
                spatial.linear_transforms,
            ),
            'afftr': (
                rng.random((*shape, 4, 4)),
                'affine_transform3',
                spatial.affine_transforms,
            ),
        }[vartype]
        # FIXME: why did we not need the cast in the scalar tests above?
        var = cast(Callable[..., sc.Variable], vals_to_var)(
            values=values, dims=['foo', 'bar', 'baz'][:ndim],
        )

    else:
        if (fold := case.endswith('-folded')):
            case, *_ = case.rpartition('-')
        vartype, _, dims = case.rpartition('-')
        ndim = int(dims.rstrip('D'))
        nentries = length ** ndim
        shape = (length,) * ndim
        if vartype == 'vecs':
            data = rng.random(nentries * 3)
            data = data.reshape((*shape, 3))
            var = sc.vectors(values=data, dims=['x', 'y', 'z'][:ndim])
            dtype = 'vector3'
        else:
            if vartype in ('strs', 'objs'):
                strings = rng.choice(
                    ['foo', 'bar', 'baz', 'foobar'], nentries,
                )
                if vartype == 'objs':
                    data = np.array([Person(s) for s in strings])
                    dtype = 'PyObject'
                else:
                    data = strings
                    dtype = 'string'
            elif vartype == 'ints':
                data = rng.choice(range(50), nentries)
                dtype = _DEFAULT_INT_TYPE
            else:
                assert vartype.startswith('float')
                dtype = vartype.rstrip('s')
                data = rng.random(nentries).astype(dtype)

            dtype_obj = getattr(sc.DType, dtype)
            dims_seq = ['x', 'y', 'z'][:ndim]
            if fold:
                # Directly converting multi-dim numpy arrays of strings
                # and non-native types can be problematic (see issues
                # #3902 and #4003), so just `.fold()` a 1-D array
                # (Also do so for some native types just to make sure
                # everything is in order)
                var = sc.array(values=data, dims=['_'], dtype=dtype_obj)
                var = var.fold('_', dims=dims_seq, shape=shape)
            else:
                data_nd = data.reshape(shape)
                var = sc.array(values=data_nd, dims=dims_seq, dtype=dtype_obj)

    assert var.dtype == getattr(sc.DType, dtype)
    assert var.shape == shape
    _roundtrip(tmp_path, var, sc.identical)


@pytest.mark.parametrize(
    'case',
    # Most of the machineries should be tested to be working via
    # `test_pickling_unbinned_variables()`.
    [
        'int-1D',
        'float-1D-with-name',
        'float-2D-with-mask',
        'vec-3D',
        'string-2D',
        'obj-2D',
        'linear-transform-1D',
    ],
)
def test_pickling_unbinned_data_arrays(
    tmp_path: Path, case: str,
) -> None:
    """
    With handpicked examples, test that un-binned :class:`DataArray`s
    can be roundtripped in- and cross-process.
    """
    rng = np.random.default_rng(42)
    length = 10
    if case == 'float-1D-with-name':
        da = sc.DataArray(
            data=sc.Variable(
                values=rng.random((length,)), dims=['t'], unit='J',
            ),
            coords={'t': sc.arange('t', 0, length, unit='s')},
            name='name',
        )
        assert da.name == 'name'
    elif case == 'float-2D-with-mask':
        da = sc.DataArray(
            data=sc.Variable(
                values=rng.random((length, length)), dims=['x', 'y'], unit='J',
            ),
            coords={
                xy: sc.arange(xy, 0, length, unit='m') for xy in ['x', 'y']
            },
            masks={
                'x': sc.Variable(
                    values=rng.choice([True, False], length).astype(bool),
                    dims=['x'],
                ),
            },
        )
        assert da.masks
    else:
        assert case.endswith(('-1D', '-2D', '-3D'))
        var_type, _, dim_ = case.rpartition('-')
        ndim = int(dim_.rstrip('D'))
        make_var: Callable[..., sc.Variable]
        shape = (length,) * ndim
        choices: Sequence[Any] | npt.NDArray[Any]
        extra_shape: tuple[int, ...]
        make_var, choices, dtype, extra_shape = {  # type: ignore[assignment]
            'int': (sc.Variable, range(20), sc.DType(np.dtype(int)), ()),
            'vec': (sc.vectors, np.linspace(0, 1), sc.DType.vector3, (3,)),
            'string': (
                sc.Variable, ['foo', 'bar', 'baz'], sc.DType.string, (),
            ),
            'obj': (
                sc.Variable,
                [Person('Alice'), Person('Bob'), Person('Chris')],
                sc.DType.PyObject,
                (),
            ),
            'linear-transform': (
                spatial.linear_transforms,
                np.linspace(0, 10),
                sc.DType.linear_transform3,
                (3, 3),
            ),
        }[var_type]

        flat_data = rng.choice(choices, (np.prod(shape), *extra_shape))
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
    _roundtrip(tmp_path, da, sc.identical)


@pytest.mark.xfail(
    reason='TODO: handle binned data', raises=NotImplementedError,
)
def test_pickling_binned_data() -> None:
    """
    Test trying to pickle a binned :class:`DataArray`. At the moment
    this should fail with a :class:`NotImplementedError`.
    """
    times = sc.array(
        values=np.random.default_rng(42).random(100), dims=['t'], unit='ms',
    )
    events = sc.DataArray(
        data=sc.ones(shape=[100], dims=['t']), coords={'t': times},
    )
    pickle.dumps(events.bin(t=10))  # FIXME: NotImplementedError


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
        mp.setattr('scipp._pickle._scipp_version', version_at_unpickling)
        msg = (
            f'expected version {version_at_unpickling}; '
            f'got version {version_at_pickling}'
        )
        with pytest.raises(VersionError, match=re.escape(msg)):
            with fname.open(mode='rb') as fobj:
                pickle.load(fobj)
