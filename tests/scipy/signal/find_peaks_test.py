import scipp as sc
from scipp.scipy.signal import find_peaks
from scipp.testing import assert_identical


def test_finds_peaks_of_dataarray() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.DataArray(sc.cos(5 * x), coords={'x': x})
    assert_identical(
        find_peaks(y),
        sc.arange(y.dim, 10, len(y), 20, unit=None),
    )


def test_finds_peaks_of_variable() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.cos(5 * x)
    assert_identical(
        find_peaks(y),
        sc.arange(y.dim, 10, len(y), 20, unit=None),
    )


def test_finds_peaks_height() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.scalar(5, unit='m') * sc.cos(5 * x)
    assert_identical(
        find_peaks(y, height=sc.scalar(6.0, unit='m')),
        # expect empty
        sc.array(dims=y.dims, values=[], dtype='int', unit=None),
    )
    assert_identical(
        find_peaks(y, height=sc.scalar(4.0, unit='m')),
        sc.arange(y.dim, 10, len(y), 20, unit=None),
    )


def test_finds_peaks_height_min_max() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.scalar(5, unit='m') * sc.cos(5 * x)
    assert_identical(
        find_peaks(y, height=(sc.scalar(4.0, unit='m'), sc.scalar(6.0, unit='m'))),
        sc.arange(y.dim, 10, len(y), 20, unit=None),
    )
    assert_identical(
        find_peaks(y, height=(sc.scalar(5.5, unit='m'), sc.scalar(6.0, unit='m'))),
        # empty
        sc.array(dims=y.dims, values=[], dtype='int', unit=None),
    )


def test_finds_peaks_height_min_max_array() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.scalar(5, unit='m') * sc.cos(5 * x)
    assert_identical(
        find_peaks(
            y,
            height=(
                sc.scalar(4, unit='m') * sc.ones(dims=['x'], shape=(101,)),
                sc.scalar(6.0, unit='m'),
            ),
        ),
        sc.arange(y.dim, 10, len(y), 20, unit=None),
    )
    assert_identical(
        find_peaks(
            y,
            height=(
                sc.scalar(5.5, unit='m') * sc.ones(dims=['x'], shape=(101,)),
                sc.scalar(6.0, unit='m'),
            ),
        ),
        # empty
        sc.array(dims=y.dims, values=[], dtype='int', unit=None),
    )


def test_finds_peaks_threshold() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.scalar(5, unit='m') * sc.cos(5 * x)
    y[50] += sc.scalar(5, unit='m')
    assert_identical(
        find_peaks(y, threshold=sc.scalar(4.0, unit='m')),
        sc.array(dims=y.dims, values=[50], dtype='int', unit=None),
    )
    assert_identical(
        find_peaks(y, threshold=sc.scalar(6.0, unit='m')),
        # expect empty
        sc.array(dims=y.dims, values=[], dtype='int', unit=None),
    )


def test_finds_peaks_threshold_min_max() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.scalar(5, unit='m') * sc.cos(5 * x)
    y[50] += sc.scalar(5, unit='m')
    assert_identical(
        find_peaks(y, threshold=(sc.scalar(4.0, unit='m'), sc.scalar(6.0, unit='m'))),
        sc.array(dims=y.dims, values=[50], dtype='int', unit=None),
    )
    assert_identical(
        find_peaks(y, threshold=(sc.scalar(5.5, unit='m'), sc.scalar(6.0, unit='m'))),
        # empty
        sc.array(dims=y.dims, values=[], dtype='int', unit=None),
    )


def test_finds_peaks_threshold_min_max_array() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.scalar(5, unit='m') * sc.cos(5 * x)
    y[50] += sc.scalar(5, unit='m')
    assert_identical(
        find_peaks(
            y,
            threshold=(
                sc.scalar(4, unit='m') * sc.ones(dims=['x'], shape=(101,)),
                sc.scalar(6.0, unit='m'),
            ),
        ),
        sc.array(dims=y.dims, values=[50], dtype='int', unit=None),
    )
    assert_identical(
        find_peaks(
            y,
            threshold=(
                sc.scalar(5.5, unit='m') * sc.ones(dims=['x'], shape=(101,)),
                sc.scalar(6.0, unit='m'),
            ),
        ),
        # empty
        sc.array(dims=y.dims, values=[], dtype='int', unit=None),
    )


def test_finds_peaks_rel_height() -> None:
    x = sc.linspace('x', -3.14, 3.14, 101, unit='rad')
    y = sc.scalar(5, unit='m') * sc.cos(5 * x)
    assert_identical(
        find_peaks(y, rel_height=sc.scalar(0.9), width=5),
        sc.arange(y.dim, 10, len(y), 20, unit=None),
    )
    assert_identical(
        find_peaks(y, rel_height=sc.scalar(0.001), width=5),
        # empty
        sc.array(dims=y.dims, values=[], dtype='int', unit=None),
    )
