import scipp as sc
import numpy as np

sets = {
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
}


def measure(value, error, unit):
    if error is None:
        return sc.scalar(value=value, unit=unit)
    return sc.scalar(value=value, variance=error**2, unit=unit)


def check_string(value, error, unit, expected):
    got = f"{measure(value, error, unit):c}"
    assert got in expected and expected in got


def test_scalar_sets():
    for s in sets:
        check_string(*s)
