# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from functools import lru_cache
from typing import Any

from ..core import DataArray, DataGroup, Variable, array, linspace, ones
from ..io import load_hdf5
from ..typing import DTypeLike

_version = '2'


@lru_cache(maxsize=1)
def _get_pooch() -> Any:
    try:
        import pooch
    except ImportError as err:
        raise ImportError(
            '''pooch is not installed.
It is required to use Scipp's bundled data files.

Please install pooch either using conda:
  conda install pooch
or using pip:
  python -m pip install pooch
or install all optional components of Scipp:
  python -m pip install scipp[all]
'''
        ) from err

    return pooch.create(
        path=pooch.os_cache('scipp'),
        env='SCIPP_DATA_DIR',
        retry_if_failed=3,
        base_url='https://public.esss.dk/groups/scipp/scipp/{version}/',
        version=_version,
        registry={
            'rhessi_flares.h5': 'md5:b4fdc9508c6d1d7aab1c6ebdd13956f2',
            'VULCAN_221040_processed.h5': 'md5:626484b95372d3341b156dc2012722f9',
        },
    )


def get_path(name: str) -> str:
    """
    Return the path to a data file bundled with Scipp.

    This function only works with example data and cannot handle
    paths to custom files.
    """
    return _get_pooch().fetch(name)  # type: ignore[no-any-return]


def rhessi_flares() -> str:
    """
    Return the path to the list of solar flares recorded by RHESSI
    in Scipp's HDF5 format.

    The original is
    https://hesperia.gsfc.nasa.gov/rhessi3/data-access/rhessi-data/flare-list/index.html

    Attention
    ---------
    This data has been manipulated!
    """
    return get_path('rhessi_flares.h5')


def vulcan_steel_strain_data() -> DataGroup[Any]:
    return load_hdf5(get_path('VULCAN_221040_processed.h5'))  # type: ignore[return-value]


def table_xyz(
    nrow: int, coord_max: float | None = None, coord_dtype: DTypeLike | None = None
) -> DataArray:
    """
    Return a 1-D data array ("table") with x, y, and z coord columns.
    """
    from numpy.random import default_rng

    rng = default_rng(seed=1234)

    def random_coordinate() -> Variable:
        return array(
            dims=['row'],
            unit='m',
            values=(
                rng.random(nrow) if coord_max is None else rng.random(nrow) * coord_max
            ),
            dtype=coord_dtype,
        )

    x = random_coordinate()
    y = random_coordinate()
    z = random_coordinate()
    data = ones(dims=['row'], unit='K', shape=[nrow])
    data.values += 0.1 * rng.random(nrow)
    return DataArray(data=data, coords={'x': x, 'y': y, 'z': z})


def binned_x(nevent: int, nbin: int) -> DataArray:
    """
    Return data array binned along 1 dimension.
    """
    return table_xyz(nevent).bin(x=nbin)


def binned_xy(nevent: int, nx: int, ny: int) -> DataArray:
    """
    Return data array binned along 2 dimensions.
    """
    return table_xyz(nevent).bin(x=nx, y=ny)


def data_xy() -> DataArray:
    from numpy.random import default_rng

    rng = default_rng(seed=1234)
    da = DataArray(array(dims=['x', 'y'], values=rng.random((100, 100))))
    da.coords['x'] = linspace('x', 0.0, 1.0, num=100, unit='mm')
    da.coords['y'] = linspace('y', 0.0, 5.0, num=100, unit='mm')
    return da
