import scipp as sc
from scipp.scipy.signal import find_peaks
from scipp.testing import assert_identical


def test_finds_peaks_of_dataarray() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.DataArray(sc.cos(5 * x), coords={'x': x})
    assert_identical(
        find_peaks(y),
        y[10::20],
    )


def test_finds_peaks_of_variable() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.cos(5 * x)
    assert_identical(
        find_peaks(y),
        y[10::20],
    )
