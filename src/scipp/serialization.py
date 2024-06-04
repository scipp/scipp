# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from .core import DataArray, Dataset, Variable


def serialize(var: Variable | DataArray | Dataset) -> tuple[dict, list[bytes]]:
    """Serialize Scipp object."""
    from io import BytesIO

    import h5py

    from .io.hdf5 import _HDF5IO

    header = {}
    buf = BytesIO()
    with h5py.File(buf, "w") as f:
        _HDF5IO.write(f, var)
    frames = [buf.getvalue()]
    return header, frames


def deserialize(header: dict, frames: list[bytes]) -> Variable | DataArray | Dataset:
    """Deserialize Scipp object."""
    from io import BytesIO

    import h5py

    from .io.hdf5 import _HDF5IO

    return _HDF5IO.read(h5py.File(BytesIO(frames[0]), "r"))


try:
    from distributed.protocol import register_serialization

    register_serialization(Variable, serialize, deserialize)
    register_serialization(DataArray, serialize, deserialize)
    register_serialization(Dataset, serialize, deserialize)
except ImportError:
    pass
__all__ = ['serialize', 'deserialize']
