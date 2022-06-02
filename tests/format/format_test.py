import scipp as sc
import numpy as np


def scalar_string(value, error, unit):
    if error is None:
        scalar = sc.scalar(value=value, unit=unit)
    else:
        scalar = sc.scalar(value=value, variance=error**2, unit=unit)
    return f"{scalar:c}"


def vector_string(value, error, unit):
    if error is None:
        vector = sc.array(values=value, unit=unit, dims=['x'])
    else:
        vector = sc.array(values=value, variances=error**2, unit=unit, dims=['x'])
    return f"{vector:c}"


def test_scalar_variables():
    scalar_variables = [
      (100, None, 's', '100 s'),
      (100., 1., 'm', '100.0(10) m'),
      (100., 2., '1', '100(2)'),
      (100., 10., 'counts', '100(10) counts'),
      (100., 100., 'us', '100(100) µs'),
      (0.01, 0.001, 'angstrom', '0.0100(10) Å'),
      (0.01, 0.002, 'cm', '0.010(2) cm'),
      (np.pi, 0.00003, 'rad', '3.14159(3) rad'),
      # default rounding rules for half:
      (234.567, 1.25, 'km', '234.6(12) km'),  # even + 0.5 -> even
      (234.567, 1.35, 'km', '234.6(14) km'),  # odd + 0.5 -> even (odd + 1)
    ]
    for value, error, unit, expected in scalar_variables:
        assert scalar_string(value, error, unit) == expected


def test_vector_variables():
    from numpy import array
    vector_variables = [
      (array([100, 20, 3]), None, 's', '100, 20, 3 s'),
      (array([100., 20.]), array([1., 2.]), 'm', '100.0(10), 20(2) m'),
      (array([9000., 800., 70., 6.]), array([100., 20., 3., 0.4]), '1',
       '9000(100), 800(20), 70(3), 6.0(4)')
    ]
    for value, error, unit, expected in vector_variables:
        assert vector_string(value, error, unit) == expected
