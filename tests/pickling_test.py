try:
    import _pickle as pickle
except ImportError:
    import pickle
import subprocess
import sys
from collections.abc import Callable
from operator import eq
from pathlib import Path
from textwrap import dedent
from typing import Any, Literal

import numpy as np
import pytest
import scipp as sc

# from scipp import Variable, DataArray, Dataset, DataGroup, DType, Unit


_64_BIT = sys.maxsize > (2 << 32)


def _roundtrip[T](
    tmpdir: Path, obj: T, cmp: Callable[[T, T], bool] | bool = True,
) -> None:
    """
    Test round-tripping ``obj`` via :mod:`pickle`.

    Test:
    - Round-tripping inside the process
    - Unpickling in a subprocess
    - Optionally, re-pickling in said subprocess, unpickling in this
      main process, and comparison with the ``obj`` via ``cmp`` (see
      below).

    Note
    ----
    ``cmp=<callable>`` compares the two arguments (``obj`` and the
    roundtripped object) and should return true(y) for success.
    ``cmp=True`` is an alias for ``cmp=operator.eq``.
    ``cmp=False`` skips re-pickling in the child process and the
    subsequent comparison of the values, only checking that the child
    can unpickle what this main process has pickled.
    """
    comparator: Callable[[T, T], bool]
    if cmp in (True, False):
        comparator = eq
        cross_proc_roundtrip = cmp
    else:
        comparator, cross_proc_roundtrip = cmp, True

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
    subprocess.run(cmd, check=True)

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
    tmp_path: Path, expected: str, dtypes: None | list[Any],
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
        mult1 = (d1 := u1.to_dict()).pop('multiplier', 1)
        mult2 = (d2 := u2.to_dict()).pop('multiplier', 1)
        if d1 != d2:
            return False
        # Coerce to Python bools to appease the typechecker
        return bool(np.isnan(mult1) and np.isnan(mult2))

    _roundtrip(tmp_path, sc.Unit(unit), cmp_units)
